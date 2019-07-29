// Primo (Privacy with Monero) Apache module

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <curl/curl.h>

#include "httpd.h"
#include "http_core.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_log.h"

#include "apr_strings.h"
#include "apr_network_io.h"
#include "apr_md5.h"
#include "apr_sha1.h"
#include "apr_hash.h"
#include "apr_base64.h"
#include "apr_dbd.h"
#include <apr_file_info.h>
#include <apr_file_io.h>
#include <apr_tables.h>
#include "util_script.h"
#include "cJSON/cJSON.h"

#define SIGNATURE_SIZE 208

typedef struct
{
  const char *host;
  const char *mining_page;
  const char *name;
  unsigned int cost;
  const char *location;
  int passthrough;
} primo_config_t;

module AP_MODULE_DECLARE_DATA primo_module;

static void init_curl()
{
  curl_global_init(CURL_GLOBAL_ALL);
}

typedef struct
{
  char *data;
  size_t bytes;
} primo_writer_t;

static size_t writef(void *contents, size_t size, size_t nmemb, void *user)
{
  primo_writer_t *w = user;
  size_t new_bytes = size * nmemb;
  w->data = realloc(w->data, w->bytes + new_bytes + 1);
  memcpy(w->data + w->bytes, contents, new_bytes);
  w->bytes += new_bytes;
  w->data[w->bytes] = 0;
  return new_bytes;
}

/* -1: error, 1: found, 0: not found */
static int find_error(const cJSON *json, const char **error_message, int *error_code)
{
  const cJSON *entry = cJSON_GetObjectItemCaseSensitive(json, "error");
  if (!entry)
    return 0;
  const cJSON *message = cJSON_GetObjectItemCaseSensitive(entry, "message");
  if (!message || !cJSON_IsString(message))
    return -1;
  *error_message = message->valuestring;
  const cJSON *code = cJSON_GetObjectItemCaseSensitive(entry, "code");
  if (!code || !cJSON_IsNumber(code))
    return -1;
  *error_code = message->valueint;
  return 1;
}

static char *call_monero(request_rec *r, const char *host, const char *method, const char *postdata)
{
  pthread_once_t once = PTHREAD_ONCE_INIT;
  pthread_once(&once, init_curl);

  CURL *curl = curl_easy_init();
  if (!curl)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: error in curl_easy_init");
    return NULL;
  }
  char url[1024];
  snprintf(url, sizeof(url), "%s/json_rpc", host);
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_POST, 1);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(postdata));
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postdata);
  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writef);
  primo_writer_t writer_data = {calloc(1, 1), 0};
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writer_data);
  struct curl_slist *list = NULL;
  list = curl_slist_append(list, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);
  int res = curl_easy_perform(curl);
  curl_slist_free_all(list); /* free the list again */
  if (res != 0)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: error in curl_easy_perform(%s): %s", url, curl_easy_strerror(res));
    free(writer_data.data);
    curl_easy_cleanup(curl);
    return NULL;
  }
  long code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if (code != 200)
  {
    free(writer_data.data);
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: HTTP status code for %s: %ld", method, code);
    curl_easy_cleanup(curl);
    return NULL;
  }
  curl_easy_cleanup(curl);

  return writer_data.data;
}

static cJSON *call_monero_and_get_result(request_rec *r, const char *host, const char *method, const char *postdata)
{
  char *response = NULL;
  cJSON *p = NULL;

  response = call_monero(r, host, method, postdata);
  if (!response)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: No response from monero server", method);
    goto error;
  }

  p = cJSON_Parse(response);
  if (!p)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC returned invalid JSON", method);
    goto error;
  }
  if (!cJSON_IsObject(p))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: Top level is not an object", method);
    goto error;
  }
  const char *error_message;
  int error_code;
  int jerror = find_error(p, &error_message, &error_code);
  if (jerror == 1)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC returned error: %s", method, error_message);
    goto error;
  }
  cJSON *result = cJSON_GetObjectItemCaseSensitive(p, "result");
  if (!result || !cJSON_IsObject(result))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC returned no result", method);
    goto error;
  }

  cJSON_DetachItemViaPointer(p, result);
  cJSON_Delete(p);

  return result;

error:
  if (p)
    cJSON_Delete(p);
  if (response)
    free(response);
  return NULL;
}

static void primo_handle_mining_page(request_rec *r, primo_config_t *conf)
{
  char postdata[128 + SIGNATURE_SIZE];
  char s[32];
  cJSON *p = NULL;
  int send_new_job = 0;
  const char *method = NULL;

  apr_table_add(r->headers_out, "X-Primo-Name", conf->name);
  apr_table_add(r->headers_out, "X-Primo-Location", conf->location);

  const char *signature = apr_table_get(r->headers_in, "X-Primo-Signature");
  if (!signature)
  {
    ap_rprintf(r, "<br>No signature found: include link to Primo explanation (in a nice page with CSS etc)<br>");
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: signature not found in headers");
    goto error;
  }
  const char *nonce = apr_table_get(r->headers_in, "X-Primo-Nonce");
  if (nonce)
  {
    const char *cookie = apr_table_get(r->headers_in, "X-Primo-Cookie");
    // could do nonce sanitization here, but monerod will apply penalties if wrong
    method = "rpc_access_submit_nonce";
    snprintf(postdata, sizeof(postdata), "{\"jsonrpc\": \"2.0\", \"method\": \"rpc_access_submit_nonce\", \"params\": {\"client\": \"%s\", \"nonce\":%u, \"cookie\":%u}, \"id\": 0}", signature, atoi(nonce), cookie ? atoi(cookie) : 0);
  }
  else
  {
    method = "rpc_access_info";
    snprintf(postdata, sizeof(postdata), "{\"jsonrpc\": \"2.0\", \"method\": \"rpc_access_info\", \"params\": {\"client\": \"%s\"}, \"id\": 0}", signature);
    send_new_job = 1;
  }

  p = call_monero_and_get_result(r, conf->host, method, postdata);
  if (!p)
    goto error;
  cJSON *result = p;

  const cJSON *entry = cJSON_GetObjectItemCaseSensitive(result, "credits");
  if (!entry || !cJSON_IsNumber(entry))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain credits", method);
    goto error;
  }
  uint64_t credits = entry->valueint;

  snprintf(s, sizeof(s), "%lu", (unsigned long)credits);
  apr_table_add(r->headers_out, "X-Primo-Credits", s);

  entry = cJSON_GetObjectItemCaseSensitive(result, "top_hash");
  if (!entry || !cJSON_IsString(entry))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain top_hash", method);
    goto error;
  }
  apr_table_add(r->headers_out, "X-Primo-TopHash", entry->valuestring);

  if (nonce)
  {
    // if the user supplied a top hash with a nonce, and if it is not the one we get back,
    // it means we're out of date, so we send back a new job
    const char *user_top_hash = apr_table_get(r->headers_in, "X-Primo-TopHash");
    if (user_top_hash && strcmp(user_top_hash, entry->valuestring))
    {
      cJSON_Delete(p);
      method = "rpc_access_info";
      snprintf(postdata, sizeof(postdata), "{\"jsonrpc\": \"2.0\", \"method\": \"rpc_access_info\", \"params\": {\"client\": \"%s\"}, \"id\": 0}", signature);
      p = call_monero_and_get_result(r, conf->host, method, postdata);
      if (!p) // if error, we can recover, just don't send job update data
        ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: error getting updated job info (ignored)", method);
      else
        send_new_job = 1;
      result = p;
    }
  }

  if (send_new_job)
  {
    entry = cJSON_GetObjectItemCaseSensitive(result, "cookie");
    if (!entry || !cJSON_IsNumber(entry))
    {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain cookie", method);
      goto error;
    }
    snprintf(s, sizeof(s), "%lu", (unsigned long)entry->valueint);
    apr_table_add(r->headers_out, "X-Primo-Cookie", s);

    entry = cJSON_GetObjectItemCaseSensitive(result, "hashing_blob");
    if (!entry || !cJSON_IsString(entry))
    {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain hashing_blob", method);
      goto error;
    }
    apr_table_add(r->headers_out, "X-Primo-HashingBlob", entry->valuestring);

    entry = cJSON_GetObjectItemCaseSensitive(result, "diff");
    if (!entry || !cJSON_IsNumber(entry))
    {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain diff", method);
      goto error;
    }
    snprintf(s, sizeof(s), "%lu", (unsigned long)entry->valueint);
    apr_table_add(r->headers_out, "X-Primo-Difficulty", s);

    entry = cJSON_GetObjectItemCaseSensitive(result, "credits_per_hash_found");
    if (!entry || !cJSON_IsNumber(entry))
    {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain credits_per_hash_found", method);
      goto error;
    }
    snprintf(s, sizeof(s), "%lu", (unsigned long)entry->valueint);
    apr_table_add(r->headers_out, "X-Primo-CreditsPerHashFound", s);

    entry = cJSON_GetObjectItemCaseSensitive(result, "height");
    if (!entry || !cJSON_IsNumber(entry))
    {
      ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain height", method);
      goto error;
    }
    snprintf(s, sizeof(s), "%lu", (unsigned long)entry->valueint);
    apr_table_add(r->headers_out, "X-Primo-Height", s);
  }

done:
  if (p)
    cJSON_Delete(p);
  return;

error:
  ap_rprintf(r, "<br>This is the Primo mining page<br>Explain here how it works, etc<br>");
  goto done;
}

static int primo_handler(request_rec *r)
{
  char s[32];
  unsigned cost = 1;
  cJSON *p = NULL;
  cJSON *result = NULL;
  const char *method = NULL;

  // check this is for us
  if(!r->handler || strcmp(r->handler, "primo-handler"))
    return DECLINED;

  // get config and sanity check values
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(r->server->module_config, &primo_module);
  if (!conf || !conf->host || !conf->mining_page)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: PrimoMoneroNode not found");
    return DONE;
  }
  if (conf->mining_page[0] != '/')
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: PrimoMiningPage does not start with /");
    ap_rprintf(r, "<br>Configuration error: PrimoMiningPage does not start with /<br>");
    return DONE;
  }

  if (strncmp(conf->location, conf->mining_page, strlen(conf->location)))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: PrimoMiningPage does not lie within PrimoLocation");
    ap_rprintf(r, "<br>Configuration error: PrimoMiningPage does not lie within PrimoLocation<br>");
    return DONE;
  }

  primo_config_t *dconf = (primo_config_t *) ap_get_module_config(r->per_dir_config, &primo_module);
  if (dconf && dconf->cost)
    cost = dconf->cost;
  else
    cost = conf->cost;

  const char *host = apr_table_get(r->headers_in, "Host");
  if (!host)
  {
    if (r->parsed_uri.is_initialized && r->parsed_uri.hostinfo)
      host = r->parsed_uri.hostinfo;
  }
  if (!host || !*host)
  { 
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: could not determine host");
    ap_rprintf(r, "<br>Configuration error: could not determine host<br>");
    return DONE;
  }

  // work out whether this is the mining page or a paywall page
  const int is_mining_page = !strcmp(conf->mining_page, r->uri);

  char mining_page_url[1024];
  snprintf(mining_page_url, sizeof(mining_page_url), "%s%s%s", strchr(host, ':') ? "" : "http://", host, conf->mining_page);
  apr_table_add(r->headers_out, "X-Primo-MiningPage", mining_page_url);

  // get signature from headers and sanity check
  const char *signature = apr_table_get(r->headers_in, "X-Primo-Signature");
  int signature_valid = 1;
  if (!signature)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: signature not found in headers");
    signature_valid = 0;
  }
  else if (strlen(signature) != SIGNATURE_SIZE)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: bad signature size (%d/%d)", (int)strlen(signature), (int)SIGNATURE_SIZE);
    signature_valid = 0;
  }

  // either mining page or paywall page
  if (is_mining_page)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: found payment page");
    primo_handle_mining_page(r, conf);
    return DONE;
  }

  // the configuration parameter was most likely forgot
  if (cost == 0)
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: cost is zero despite being configured");
    goto payment_required;
  }

  // paywall page, include the cost in a response header
  snprintf(s, sizeof(s), "%u", cost);
  apr_table_add(r->headers_out, "X-Primo-Payment", s);

  if (!signature_valid)
    goto payment_required;

  // call the pay RPC
  char postdata[128 + SIGNATURE_SIZE];
  snprintf(postdata, sizeof(postdata), "{\"jsonrpc\": \"2.0\", \"method\": \"rpc_access_pay\", \"params\": {\"client\": \"%s\", \"paying_for\":\"%s\", \"payment\":%u}, \"id\": 0}", signature, "test", cost);
  method = "rpc_access_pay";
  p = call_monero_and_get_result(r, conf->host, "rpc_access_pay", postdata);
  if (!p)
    goto payment_required;
  result = p;

  // include credits left in a response header
  cJSON *entry = cJSON_GetObjectItemCaseSensitive(result, "credits");
  if (!entry || !cJSON_IsNumber(entry))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain credits", method);
    goto payment_required;
  }
  uint64_t credits = entry->valueint;
  snprintf(s, sizeof(s), "%lu", (unsigned long)credits);
  apr_table_add(r->headers_out, "X-Primo-Credits", s);

  // check whether we got the OK or not
  entry = cJSON_GetObjectItemCaseSensitive(result, "status");
  if (!entry || !cJSON_IsString(entry))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: JSON does not contain status", method);
    goto payment_required;
  }
  if (strcmp(entry->valuestring, "OK"))
  {
    ap_log_error(APLOG_MARK, APLOG_ERR, 0, r->server, "primo: %s RPC: Payment not accepted", method);
    goto payment_required;
  }

  cJSON_Delete(p);

  // Access granted
  return DECLINED;

payment_required:
  if (p)
    cJSON_Delete(p);
  if (conf->passthrough)
  {
    // Access passthrough, the server will handle the request, we just add a header so it can decide what to do
    apr_table_add(r->headers_out, "X-Primo-Unpaid", "1");
    return DECLINED;
  }
  else
  {
    // Reject with 402 - Payment required
    ap_rprintf(r, "Access denied by Primo<br>See %s for information.<br>", conf->mining_page);
    r->status = HTTP_PAYMENT_REQUIRED;
    return DONE;
  }
}


static const char *set_monero_host(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(cmd->server->module_config, &primo_module);
  if (!conf)
    return "primo: server structure not allocated";
  conf->host = arg;
  return NULL;
}

static const char *set_mining_page(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(cmd->server->module_config, &primo_module);
  if (!conf)
    return "primo: server structure not allocated";
  conf->mining_page = arg;
  return NULL;
}

static const char *set_name(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(cmd->server->module_config, &primo_module);
  if (!conf)
    return "primo: server structure not allocated";
  conf->name = arg;
  return NULL;
}

static const char *set_location(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(cmd->server->module_config, &primo_module);
  if (!conf)
    return "primo: server structure not allocated";
  conf->location = arg;
  return NULL;
}

static const char *set_passthrough(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *) ap_get_module_config(cmd->server->module_config, &primo_module);
  if (!conf)
    return "primo: server structure not allocated";
  conf->passthrough = atoi(arg);
  return NULL;
}

static const char *set_cost(cmd_parms *cmd, void *cfg, const char *arg)
{
  primo_config_t *conf = (primo_config_t *)cfg;
  if (!conf)
    return "primo: server structure not allocated";
  conf->cost = atoi(arg);
  return NULL;
}

static const command_rec mod_primo_cmds[] =
{
  AP_INIT_TAKE1("PrimoMoneroHost", set_monero_host, NULL, RSRC_CONF,
    "Hostname where the Monero node is listening"),
  AP_INIT_TAKE1("PrimoMiningPage", set_mining_page, NULL, RSRC_CONF,
    "Page that handles payments"),
  AP_INIT_TAKE1("PrimoCost", set_cost, NULL, ACCESS_CONF,
    "Cost per request"),
  AP_INIT_TAKE1("PrimoLocation", set_location, NULL, RSRC_CONF,
    "Location of pages to which this cost applies"),
  AP_INIT_TAKE1("PrimoName", set_name, NULL, RSRC_CONF,
    "Name of the website to display to the user"),
  AP_INIT_TAKE1("PrimoPassthrough", set_passthrough, NULL, RSRC_CONF,
    "Whether to pass on requests even if the user's balance is not enough"),
  {NULL}
};

/*
 * configuration file commands - exported to Apache API
 */

static void *mod_primo_config_handler(apr_pool_t *p, server_rec *d)
{
    /* allocate the config - use pcalloc because it needs to be zeroed */
    return apr_pcalloc(p, sizeof(primo_config_t));
}

static void *mod_primo_config_handler_per_dir(apr_pool_t *p, char *d)
{
    /* allocate the config - use pcalloc because it needs to be zeroed */
    return apr_pcalloc(p, sizeof(primo_config_t));
}

static void *mod_primo_config_merge_handler(apr_pool_t *p, void *basev, void *addv)
{
    primo_config_t *base = (primo_config_t *) basev;
    primo_config_t *add = (primo_config_t *) addv;
    primo_config_t *new = (primo_config_t *) apr_palloc(p, sizeof(primo_config_t));

    new->host = add->host ? add->host : base->host;
    new->mining_page = add->mining_page ? add->mining_page : base->mining_page;
    new->cost = add->cost ? add->cost : base->cost;
    new->name = add->name ? add->name : base->name;
    new->location = add->location ? add->location : base->location;
    new->passthrough = add->passthrough ? add->passthrough : base->passthrough;
    return new;
}

/* register_hooks: Adds a hook to the httpd process */
static void register_hooks(apr_pool_t *pool) 
{
  /* Hook the request handler */
  ap_hook_handler(primo_handler, NULL, NULL, APR_HOOK_LAST);
}

module AP_MODULE_DECLARE_DATA primo_module =
{ 
  STANDARD20_MODULE_STUFF,
  mod_primo_config_handler_per_dir, /* Per-directory configuration handler */
  mod_primo_config_merge_handler, /* Merge handler for per-directory configurations */
  mod_primo_config_handler, /* Per-server configuration handler */
  mod_primo_config_merge_handler, /* Merge handler for per-server configurations */
  mod_primo_cmds, /* Any directives we may have for httpd */
  register_hooks   /* Our hook registering function */
};

