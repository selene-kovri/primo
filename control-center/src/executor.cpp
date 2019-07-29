/*  Primo - Privacy with Monero
 *
 *  Copyright (C) 2019 selene
 *
 *  This file is part of Primo.
 *
 *  Primo is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Primo is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Primo.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdexcept>
#include <QGuiApplication>
#include <QWindow>
#include "cJSON/cJSON.h"
#include "signature.h"
#include "db.h"
#include "miner.h"
#include "executor.h"

#include <QList>

Executor::Executor(QGuiApplication *gui, DB *db, Miner *miner):
  QObject(gui),
  gui(gui),
  db(db),
  miner(miner),
  found_nonce(0)
{
  connect(miner, SIGNAL(hashFound(const std::string&, uint32_t, const std::string&, uint32_t)), this, SLOT(onHashFound(const std::string&, uint32_t, const std::string&, uint32_t)));
}

std::string Executor::cmd_site(const std::string &id, const std::string &mining_page, const std::string &location, const std::string &name, const uint64_t *payment, const uint64_t *credits, const std::string &hashing_blob, const uint64_t *difficulty, const uint64_t *credits_per_hash_found, const uint64_t *height, const std::string &top_hash, const uint32_t *cookie)
{
  bool is_new = db->update(mining_page, location, name, payment, credits, hashing_blob, difficulty, credits_per_hash_found, height, top_hash, cookie);
  if (!mining_page.empty() && !hashing_blob.empty() && difficulty && height && !top_hash.empty() && cookie)
    miner->update(mining_page, hashing_blob, *difficulty, *height, top_hash, *cookie);
  return "{\"id\":\""+id+"\", \"is_new\":"+(is_new ? "true" : "false")+"}";
}

std::string Executor::cmd_sign(const std::string &id, const std::string &url)
{
  std::string s = db->make_signature(url);
  return "{\"id\":\""+id+"\", \"signature\":\""+s+"\"}";
}

std::string Executor::cmd_forget_all(const std::string &id)
{
  db->deleteAllEntries();
  return "{\"id\":\""+id+"\"}";
}

std::string Executor::cmd_summon(const std::string &id)
{
  QList<QWindow*> l = gui->topLevelWindows();
  if (!l.isEmpty())
  {
    QWindow *window = l.front();
    if (window)
    {
      window->show();
      window->raise();
      window->requestActivate();
    }
  }
  return "{\"id\":\""+id+"\"}";
}

std::string Executor::execute(const std::string &command)
{
  std::string reply;
  std::string id = "0";
  cJSON *p = NULL;
  try
  {
    cJSON *p = cJSON_Parse(command.c_str());
    if (!p)
      throw std::runtime_error("Failed to parse JSON");

    // for some reason, we have to decode twice, not sure why
    if (cJSON_IsString(p))
    {
      std::string s = p->valuestring;
      cJSON_Delete(p);
      p = cJSON_Parse(s.c_str());
      if (!p)
        throw std::runtime_error("Failed to parse JSON");
    }

    if (!cJSON_IsObject(p))
      throw std::runtime_error("Top level is not an object: " + std::to_string(p->type));
    const cJSON *entry = cJSON_GetObjectItemCaseSensitive(p, "id");
    if (entry)
    { 
      if (cJSON_IsString(entry))
        id = entry->valuestring;
      else if (cJSON_IsNumber(entry))
        id = std::to_string(entry->valueint);
    }
    entry = cJSON_GetObjectItemCaseSensitive(p, "cmd");
    if (!entry || !cJSON_IsString(entry))
      throw std::runtime_error("Failed to find cmd field");
    std::string cmd = entry->valuestring;
    if (cmd == "site")
    {
      uint64_t payment_data = 0, credits_data = 0, difficulty_data = 0, credits_per_hash_found_data = 0, height_data = 0;
      const uint64_t *payment = NULL, *credits = NULL, *difficulty = NULL, *credits_per_hash_found = NULL, *height = NULL;
      uint32_t cookie_data = 0;
      const uint32_t *cookie = NULL;
      std::string location, name, hashing_blob, top_hash;

      entry = cJSON_GetObjectItemCaseSensitive(p, "mining_page");
      if (!entry || !cJSON_IsString(entry))
        throw std::runtime_error("mining_page not found");
      const char *mining_page = entry->valuestring;

      entry = cJSON_GetObjectItemCaseSensitive(p, "payment");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("payment is not a number");
        payment_data = entry->valueint;
        payment = &payment_data;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "credits");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("credits is not a number");
        credits_data = entry->valueint;
        credits = &credits_data;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "location");
      if (entry)
      {
        if (!cJSON_IsString(entry))
          throw std::runtime_error("location is not a string");
        location = entry->valuestring;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "name");
      if (entry)
      {
        if (!cJSON_IsString(entry))
          throw std::runtime_error("name is not a string");
        name = entry->valuestring;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "hashing_blob");
      if (entry)
      {
        if (!cJSON_IsString(entry))
          throw std::runtime_error("hashing_blob is not a string");
        hashing_blob = entry->valuestring;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "difficulty");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("difficulty is not a number");
        difficulty_data = entry->valueint;
        difficulty = &difficulty_data;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "credits_per_hash_found");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("credits_per_hash_found is not a number");
        credits_per_hash_found_data = entry->valueint;
        credits_per_hash_found = &credits_per_hash_found_data;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "height");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("height is not a number");
        height_data = entry->valueint;
        height = &height_data;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "top_hash");
      if (entry)
      {
        if (!cJSON_IsString(entry))
          throw std::runtime_error("top_hash is not a string");
        top_hash = entry->valuestring;
      }

      entry = cJSON_GetObjectItemCaseSensitive(p, "cookie");
      if (entry)
      {
        if (!cJSON_IsNumber(entry))
          throw std::runtime_error("cookie is not a number");
        cookie_data = entry->valueint;
        cookie = &cookie_data;
      }

      reply = cmd_site(id, mining_page, location, name, payment, credits, hashing_blob, difficulty, credits_per_hash_found, height, top_hash, cookie);
    }
    else if (cmd == "sign")
    {
      entry = cJSON_GetObjectItemCaseSensitive(p, "url");
      if (!entry || !cJSON_IsString(entry))
        throw std::runtime_error("url not found");
      const char *url = entry->valuestring;
      reply = cmd_sign(id, url);
    }
    else if (cmd == "forget_all")
    {
      reply = cmd_forget_all(id);
    }
    else if (cmd == "summon")
    {
      reply = cmd_summon(id);
    }
    else
    {
      throw std::runtime_error("Invalid command: " + std::string(entry->valuestring));
    }
    cJSON_Delete(p);
  }
  catch (const std::exception &e)
  {
    reply = "{\"id\":"+id+", \"error\":{\"message\":\""+e.what()+"\"}}";
    if (p)
      cJSON_Delete(p);
  }
  return reply;
}

std::string Executor::tick()
{
  DB::Locker locker(db);
  const unsigned n_entries = db->get_num_entries();
  const time_t now = time(NULL);
  unsigned want_to_mine_index = n_entries;
  unsigned currently_mining_index = n_entries;
  unsigned mine_index = n_entries;
  const std::string mining_for = miner->get_mining_page();

  // find out what we want to mine for - any that matches for now
  for (unsigned i = 0; i < n_entries; ++i)
  {
    DB::Entry e = db->get_entry(i);
    if (e.enabled && e.balance < e.target && !e.hashing_blob.empty() && e.difficulty > 0)
      want_to_mine_index = i;
    if (e.mining_page == mining_for)
      currently_mining_index = i;
  }

  // get info for what we need
  for (unsigned i = 0; i < n_entries; ++i)
  {
    DB::Entry e = db->get_entry(i);
    if (e.last_info_query_time + 10 <= now && (want_to_mine_index == i || currently_mining_index == i || e.name.empty()))
    {
      db->setInfoQueried(i);
      return "{\"cmd\":\"info\",\"mining_page\":\""+e.mining_page+"\"}";
    }
  }

  // decide what to actually mine, we might not be able to mine on what we want just yet
  mine_index = want_to_mine_index;
  for (int i = 0; i < 2; ++i)
  {
    if (mine_index != n_entries)
    {
      DB::Entry e = db->get_entry(mine_index);
      if (!e.enabled || e.mining_page.empty() || e.hashing_blob.empty() || e.difficulty == 0 || e.last_hashing_blob_time + 15 < now || e.balance >= e.target)
      {
        mine_index = currently_mining_index;
      }
    }
  }

  if (mine_index != n_entries)
  {
    DB::Entry e = db->get_entry(mine_index);
    if (mining_for != e.mining_page)
      fprintf(stderr, "Mining for %s on %s\n", e.name.c_str(), e.mining_page.c_str());
    miner->set_hashing_blob(e.hashing_blob, e.difficulty, e.height, e.top_hash, e.cookie, e.mining_page, e.name);
  }
  else
  {
    if (!mining_for.empty())
      fprintf(stderr, "Stopping mining\n");
    miner->stop();
  }
  return "";
}

void Executor::onHashFound(const std::string &mining_page, uint32_t nonce, const std::string &top_hash, uint32_t cookie)
{
  found_mining_page = mining_page;
  found_nonce = nonce;
  found_top_hash = top_hash;
  found_cookie = cookie;
  if (!db->has_entry(found_mining_page))
    return;
  emit hashFound();
}

std::string Executor::submit_nonce()
{
  return "{\"cmd\":\"nonce\", \"mining_page\": \""+found_mining_page+"\", \"nonce\": "+std::to_string(found_nonce)+", \"top_hash\":\""+found_top_hash+"\", \"cookie\":"+std::to_string(found_cookie)+"}";
}
