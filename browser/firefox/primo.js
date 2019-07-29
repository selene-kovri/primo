var reqid = 0;
var port = setup_port()
var debug = false

function setup_port()
{
  var port = browser.runtime.connectNative("primo.control.center")
  port.onMessage.addListener((data) => {
    if (!data.id)
    {
      // we got a command, not a reply
      if (data.cmd == "info")
      {
        var mining_page = data.mining_page
        if (debug)
          console.log("Fetching info from " + mining_page)
  
        var xmlhttp;
        if (window.XMLHttpRequest) {
            xmlhttp = new XMLHttpRequest();
            xmlhttp.onreadystatechange = function() {
              if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
                if (debug)
                  console.log("Response from mining info query:" + xmlhttp.responseText);
              }
            }
            xmlhttp.open("GET", mining_page, true);
            xmlhttp.send(null);
        }
        else
          console.warn("XMLHttpRequest not found")
      }
      else if (data.cmd == "nonce")
      {
        var mining_page = data.mining_page
        var nonce = data.nonce
        var top_hash = data.top_hash
        var cookie = data.cookie
        console.log("Submitting nonce to " + mining_page)
  
        var xmlhttp;
        if (window.XMLHttpRequest) {
            xmlhttp = new XMLHttpRequest();
            xmlhttp.onreadystatechange = function() {
              if(xmlhttp.readyState === 4 && xmlhttp.status === 200) {
                console.log(xmlhttp.responseText);
              }
            }
            xmlhttp.open("POST", mining_page, true);
            xmlhttp.setRequestHeader("X-Primo-Nonce", nonce);
            xmlhttp.setRequestHeader("X-Primo-TopHash", top_hash);
            xmlhttp.setRequestHeader("X-Primo-Cookie", cookie);
            xmlhttp.send(null);
        }
        else
          console.warn("XMLHttpRequest not found")
      }
      else if (data.cmd == "url")
      {
        browser.tabs.create({url: data.url})
      }
      else
        console.warn("Got unknown command: " + JSON.stringify(data))
    }
  });
  return port
}

function callbackOnId(ev, id, callback) {
  var listener = ( function(port, id) {
    var handler = function(msg) {
      if(msg.id == id) {
        ev.removeListener(handler);
        callback(msg);
      }
    }
    return handler;
  })(ev, id, callback);
  ev.addListener(listener);
}

function summon() {
  if (debug)
    console.log("Summoning control center");
  var tid = window.setTimeout(function() {
    console.warn("Not responding, restarting")
    setup_port()
  }, 500)
  var id = reqid++;
  callbackOnId(port.onMessage, id, function(response) {
    window.clearTimeout(tid)
  })
  port.postMessage("{\"cmd\":\"summon\", \"id\":"+id+"}")
}

function onHeadersReceived(e)
{
  var i;
  var mining_page, payment, credits, location, name, hashing_blob, difficulty, credits_per_hash_found, height, top_hash, cookie;
  if (debug)
    console.log("Got reply, with " + e.responseHeaders.length + " headers:")
  for (i = 0; i < e.responseHeaders.length; ++i)
  {
    if (debug)
      console.log("  " + e.responseHeaders[i].name + ": " + e.responseHeaders[i].value);
    if (e.responseHeaders[i].name == "X-Primo-MiningPage")
      mining_page = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Payment")
      payment = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Credits")
      credits = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Name")
      name = decodeURIComponent(escape(e.responseHeaders[i].value));
    else if (e.responseHeaders[i].name == "X-Primo-Location")
      location = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-HashingBlob")
      hashing_blob = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Difficulty")
      difficulty = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-CreditsPerHashFound")
      credits_per_hash_found = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Height")
      height = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-TopHash")
      top_hash = e.responseHeaders[i].value;
    else if (e.responseHeaders[i].name == "X-Primo-Cookie")
      cookie = e.responseHeaders[i].value;
  }

  if (mining_page)
  {
    var id = reqid++;
    callbackOnId(port.onMessage, id, function(response) {
      if (debug)
        console.log("Response for id " + id + ": " + JSON.stringify(response))
      if (response.is_new && name)
      {
        var nid = browser.notifications.create("new-site-" + mining_page, {
          "type": "basic",
          "iconUrl": browser.runtime.getURL("icons/monero48.png"),
          "title": "Primo",
          "message": name + " is requesting payment for access\n\nClick to summon Primo",
        });
        browser.notifications.onClosed.addListener(function(notificationId) {
          if (notificationId == nid)
            window.clearTimeout(tid)
        });
        browser.notifications.onClicked.addListener(function(notificationId) {
          if (notificationId = nid)
          {
            browser.notifications.clear("new-site-" + mining_page);
            summon()
          }
        });
        var tid = window.setTimeout(function() {
          browser.notifications.clear("new-site-" + mining_page)
        }, 15000);
      }
    })
    json = "{\"cmd\":\"site\", \"id\": "+id+", \"mining_page\":\""+mining_page+"\"";
    if (payment)
      json += ", \"payment\":"+payment;
    if (credits)
      json += ", \"credits\":"+credits;
    if (location)
      json += ", \"location\":\""+location + "\"";
    if (name)
      json += ", \"name\":\""+name + "\"";
    if (hashing_blob)
      json += ", \"hashing_blob\":\""+hashing_blob + "\"";
    if (difficulty)
      json += ", \"difficulty\":"+difficulty;
    if (credits_per_hash_found)
      json += ", \"credits_per_hash_found\":"+credits_per_hash_found;
    if (height)
      json += ", \"height\":"+height;
    if (top_hash)
      json += ", \"top_hash\":\""+top_hash+"\"";
    if (cookie)
      json += ", \"cookie\":"+cookie;
    json += "}";
    if (debug)
      console.log("Posting json for id " + id + ": " + json);
    port.postMessage(json);
  }
}

function onBeforeSendHeaders(e)
{
  if (debug)
    console.log("We need to add a signature for " + e.url)
  return new Promise((resolve, reject) => {
    var tid = window.setTimeout(function() {
      console.warn("Not responding, sending headers without signature")
      reject("primo-control-center not responding")
    }, 2000)
    var id = reqid++;
    callbackOnId(port.onMessage, id, function(response) {
      window.clearTimeout(tid)
      signature = response.signature
      if (debug)
        console.log("We got signature: " + signature)
      e.requestHeaders.push({name: "X-Primo-Signature", value: signature})
      resolve({requestHeaders: e.requestHeaders})
    })
    port.postMessage("{\"cmd\":\"sign\", \"id\":"+id+", \"url\":\""+e.url+"\"}")
  });
}

browser.webRequest.onHeadersReceived.addListener(
  onHeadersReceived,
  {urls: ["<all_urls>"]},
  ["responseHeaders"]
)

browser.webRequest.onBeforeSendHeaders.addListener(
  onBeforeSendHeaders,
  {urls: ["<all_urls>"]},
  ["blocking", "requestHeaders"]
);

browser.browserAction.onClicked.addListener(() => {
  summon()
});
