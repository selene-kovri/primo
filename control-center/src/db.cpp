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

#include <boost/algorithm/string.hpp>
#include <QUrl>
#include "signature.h"
#include "model.h"
#include "db.h"

DB::Entry::Entry(const std::string &mining_page, const std::string &location, const std::string &name):
  mining_page(mining_page),
  location(location),
  name(name),
  payment(0),
  balance(0),
  target(0),
  enabled(true),
  last_info_query_time(0),
  difficulty(0),
  credits_per_hash_found(0),
  height(0),
  cookie(0),
  last_hashing_blob_time(0)
{
  crypto::random32_unbiased((unsigned char*)skey.data);
}

DB::DB()
{
#if 0
  update("url1", "location1", "The 范冰冰 Website", 0, NULL, "", NULL, NULL, NULL, "", NULL);
  update("url2", "location2", "Foobar", 0, NULL, "", NULL, NULL, NULL, "", NULL);

  uint64_t payment = 100, balance = 0, difficulty = 50, credits_per_hash_found = 20, height = 1;
  uint32_t cookie = 0;
  update("mining_url", "mining site", "Mining site", &payment, &balance, "0b0bfa9bf9e805418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e30000000099ca9f46a9e27cee2e0292d6fbd86033fc33a89ba908790b57792515dca99ef801", &difficulty, &credits_per_hash_found, &height, "418015bb9ae982a1975da7d79277c2705727a56894ba0fb246adaabb1f4632e3", &cookie);
#endif
}

void DB::lock()
{
  mutex.lock();
}

void DB::unlock()
{
  mutex.unlock();
}

int DB::find_mining_page(const std::string &mining_page)
{
  DB::Locker locker(this);
  for (size_t idx = 0; idx < db.size(); ++idx)
  {
    if (mining_page == db[idx].mining_page)
      return idx;
  }
  return -1;
}

int DB::find_url(const std::string &url)
{
  const QUrl qurl(QString::fromStdString(url));
  const std::string path = qurl.path().toStdString();
  DB::Locker locker(this);
  for (size_t idx = 0; idx < db.size(); ++idx)
  {
    if (url == db[idx].mining_page || (!db[idx].location.empty() && boost::starts_with(path, db[idx].location)))
      return idx;
  }
  return -1;
}

bool DB::update(const std::string &mining_page, const std::string &location, const std::string &name, const uint64_t *payment, const uint64_t *balance, const std::string &hashing_blob, const uint64_t *difficulty, const uint64_t *credits_per_hash_found, const uint64_t *height, const std::string &top_hash, const uint32_t *cookie)
{
  bool is_new = false;
  DB::Locker locker(this);
  int idx = find_mining_page(mining_page);
  if (idx < 0)
  {
    db.push_back(Entry(mining_page, location, name));
    idx = db.size() - 1;
    balance = NULL; // the first hit is often cached, so balance may be wrong, and we're going to hit the mining page so we'll get actual balance anyway
    is_new = true;
  }
  if (!location.empty())
    db[idx].location = location;
  if (!name.empty())
  {
    if (db[idx].name != name)
      is_new = true;
    db[idx].name = name;
  }
  if (payment)
    db[idx].payment = *payment;
  if (balance)
    db[idx].balance = *balance;
  if (!hashing_blob.empty())
  {
    db[idx].hashing_blob = hashing_blob;
    db[idx].last_hashing_blob_time = time(NULL);
  }
  if (difficulty)
    db[idx].difficulty = *difficulty;
  if (credits_per_hash_found)
    db[idx].credits_per_hash_found = *credits_per_hash_found;
  if (height)
    db[idx].height = *height;
  if (!top_hash.empty())
    db[idx].top_hash = top_hash;
  if (cookie)
    db[idx].cookie = *cookie;
  emit modelChanged();
  if (is_new)
    emit newSite(QString::fromStdString(db[idx].mining_page), QString::fromStdString(db[idx].name));
  return is_new;
}

std::string DB::make_signature(const std::string &url)
{
  DB::Locker locker(this);
  int idx = find_url(url);
  if (idx < 0)
    return "";
  return ::make_signature(db[idx].skey);
}

size_t DB::get_num_entries()
{
  DB::Locker locker(this);
  return db.size();
}

DB::Entry DB::get_entry(unsigned int idx)
{
  DB::Locker locker(this);
  if (idx >= db.size())
    return {};
  return db[idx];
}

bool DB::has_entry(const std::string &mining_page)
{
  return find_mining_page(mining_page) >= 0;
}

void DB::deleteEntry(unsigned idx)
{
  DB::Locker locker(this);
  if (idx >= db.size())
    return;
  db.erase(db.begin() + idx);
  emit modelChanged();
}

void DB::deleteAllEntries()
{
  DB::Locker locker(this);
  db.clear();
  emit modelChanged();
}

void DB::setTarget(unsigned idx, quint64 target)
{
  DB::Locker locker(this);
  if (idx >= db.size())
    return;
  db[idx].target = target;
  emit modelChanged();
}

void DB::setEnabled(unsigned idx, bool enabled)
{
  DB::Locker locker(this);
  if (idx >= db.size())
    return;
  db[idx].enabled = enabled;
  emit modelChanged();
}

void DB::setInfoQueried(unsigned idx)
{
  DB::Locker locker(this);
  if (idx >= db.size())
    return;
  db[idx].last_info_query_time = time(NULL);
  emit modelChanged();
}
