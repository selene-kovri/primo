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

#include <stdio.h>
#include <boost/optional.hpp>
#include "int-util.h"
#include "difficulty.h"
#include "miner.h"

#include "string_tools.h"
#define HASH_RATE_UPDATE_TIME 1 /* seconds */

Miner::Miner(QObject *parent):
  QObject(parent),
  difficulty(0),
  height(0),
  cookie(0),
  running(false),
  n_hashes(0),
  hash_rate_window_start_time(boost::date_time::not_a_date_time),
  hash_rate_window_start_hashes(0),
  is_paused(false),
  hash_rate_history(16)
{
  running = true;
  thread = boost::thread([this]() { mining_thread(); } );
}

Miner::~Miner()
{
  {
    boost::unique_lock<boost::mutex> lock(mutex);
    running = false;
    cond.notify_one();
  }
  thread.join();
}

void Miner::mining_thread()
{
  static uint32_t nonce = 0;
  uint64_t local_difficulty = 0, local_height = 0;
  std::string local_top_hash, local_mining_page;
  std::vector<uint8_t> local_raw_hashing_blob;
  uint32_t local_cookie = std::numeric_limits<uint32_t>::max();

  while (1)
  {
    {
      boost::unique_lock<boost::mutex> lock(mutex);
      if (!running)
        break;
      if (is_paused || hashing_blob.empty())
      {
        cond.wait(lock);
        continue;
      }
      if (cookie != local_cookie)
      {
        local_raw_hashing_blob = raw_hashing_blob;
        local_difficulty = difficulty;
        local_height = height;
        local_top_hash = top_hash;
        local_cookie = cookie;
        local_mining_page = mining_page;
fprintf(stderr, "Hashing for diff %u, height %u, top hash %s\n", (unsigned)local_difficulty, (unsigned)local_height, local_top_hash.c_str());
      }
    }

    // hash once
    int cn_variant = local_raw_hashing_blob[0] >= 7 ? local_raw_hashing_blob[0] - 6 : 0;
    ++nonce;
    *(uint32_t*)(local_raw_hashing_blob.data() + 39) = SWAP32LE(nonce);
    crypto::hash hash;
    crypto::cn_slow_hash(local_raw_hashing_blob.data(), local_raw_hashing_blob.size(), hash, cn_variant, local_height);
    const bool found = check_hash(hash, local_difficulty);
    if (found)
    {
      fprintf(stderr, "Found hash %u for height %u, CNv%u\n", nonce, (unsigned)local_height, cn_variant);
      fprintf(stderr, "  -> top hash %s\n", local_top_hash.c_str());
      fprintf(stderr, "  -> diff %u, pow %s\n", (unsigned)local_difficulty, epee::string_tools::pod_to_hex(hash).c_str());
      emit hashFound(local_mining_page, nonce, local_top_hash, local_cookie);
    }

    boost::unique_lock<boost::mutex> lock(mutex);
    ++n_hashes;
    const boost::posix_time::ptime now = boost::posix_time::microsec_clock::universal_time();
    if (now > hash_rate_window_start_time + boost::posix_time::seconds(HASH_RATE_UPDATE_TIME))
    {
      float current_hash_rate = 1000.0f * (n_hashes - hash_rate_window_start_hashes) / (float)((now - hash_rate_window_start_time).total_milliseconds());
      hash_rate_history.push_back(current_hash_rate);
      hash_rate_window_start_time = now;
      hash_rate_window_start_hashes = n_hashes;
      float new_hash_rate = get_hash_rate_average();
      lock.unlock();
      emit hashRateChanged(new_hash_rate);
    }
  }
}

void Miner::set_hashing_blob(const std::string &new_hashing_blob, uint64_t new_difficulty, uint64_t new_height, const std::string &new_top_hash, uint32_t new_cookie, const std::string &new_mining_page, const std::string &new_name, bool keep_mining_page)
{
  boost::optional<float> hash_rate_changed;
  boost::optional<std::string> mining_page_changed;
  boost::optional<std::string> name_changed;

  {
    boost::unique_lock<boost::mutex> lock(mutex);
    bool was_empty = hashing_blob.empty();
    if (keep_mining_page && mining_page != new_mining_page)
      return;
    hashing_blob = new_hashing_blob;
    try
    {
      raw_hashing_blob = epee::from_hex::vector(hashing_blob);
    }
    catch (...)
    {
      fprintf(stderr, "Invalid hashing_blob\n");
      hashing_blob.clear();
      raw_hashing_blob.clear();
    }
    if (!raw_hashing_blob.empty() && raw_hashing_blob.size() < 43)
    {
      fprintf(stderr, "Invalid hashing_blob size (%zu)\n", raw_hashing_blob.size());
      hashing_blob.clear();
      raw_hashing_blob.clear();
    }
    difficulty = new_difficulty;
    height = new_height;
    top_hash = new_top_hash;
    cookie = new_cookie;
    if (mining_page != new_mining_page)
    {
      mining_page = new_mining_page;
      mining_page_changed = new_mining_page;
    }
    if (!new_name.empty() && mining_name != new_name)
    {
      mining_name = new_name;
      name_changed = new_name;
    }

    if (hashing_blob.empty())
    {
      hash_rate_window_start_time = boost::posix_time::microsec_clock::universal_time();
      hash_rate_window_start_hashes = n_hashes;
      hash_rate_history.clear();
      hash_rate_changed = 0.0f;
    }
    else if (was_empty)
    {
      hash_rate_window_start_time = boost::posix_time::microsec_clock::universal_time();
      hash_rate_window_start_hashes = n_hashes;
    }
    cond.notify_one();
  }

  if (hash_rate_changed)
    emit hashRateChanged(*hash_rate_changed);
  if (mining_page_changed)
    emit miningPageChanged(QString::fromStdString(*mining_page_changed));
  if (name_changed)
    emit nameChanged(QString::fromStdString(*name_changed));
}

void Miner::update(const std::string &new_mining_page, const std::string &new_hashing_blob, uint64_t new_difficulty, uint64_t new_height, const std::string &new_top_hash, uint32_t new_cookie)
{
  set_hashing_blob(new_hashing_blob, new_difficulty, new_height, new_top_hash, new_cookie, new_mining_page, "", true);
}

void Miner::stop()
{
  set_hashing_blob("", 0, 0, "", 0, "", "");
}

void Miner::pause(bool p)
{
  bool notify_pause_changed, notify_hash_rate_changed, was_paused;
  float notify_hash_rate = 0.0f;
  {
    boost::unique_lock<boost::mutex> lock(mutex);
    was_paused = is_paused;
    is_paused = p;
    notify_pause_changed = is_paused != was_paused;
    notify_hash_rate_changed = is_paused && !was_paused;
    if (is_paused)
    {
      hash_rate_window_start_time = boost::posix_time::microsec_clock::universal_time();
      hash_rate_window_start_hashes = n_hashes;
      hash_rate_history.clear();
      notify_hash_rate = 0.0f;
    }
  }

  if (notify_pause_changed)
    emit pausedChanged();
  if (notify_hash_rate_changed)
    emit hashRateChanged(notify_hash_rate);
}

bool Miner::paused()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  return is_paused;
}

float Miner::get_hash_rate_average() const
{
  if (hash_rate_history.empty())
    return 0.0f;
  float average = 0.0f;
  for (float x: hash_rate_history)
    average += x;
  average /= hash_rate_history.size();
  return average;
}

float Miner::hashRate()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  return get_hash_rate_average();
}

std::string Miner::get_mining_page()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  return mining_page;
}

QString Miner::miningPage()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  return QString::fromStdString(mining_page);
}

QString Miner::miningName()
{
  boost::unique_lock<boost::mutex> lock(mutex);
  return QString::fromStdString(mining_name);
}
