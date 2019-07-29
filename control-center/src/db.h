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

#ifndef DB_H
#define DB_H

#include <string>
#include <deque>
#include <boost/thread/recursive_mutex.hpp>
#include <QObject>
#include "crypto/crypto.h"

#include <QtWidgets/QWidget>

class Model;

class DB: public QObject
{
  Q_OBJECT

public:
  struct Entry
  {
    std::string mining_page;
    std::string location;
    std::string name;
    uint64_t payment;
    uint64_t balance;
    uint64_t target;
    bool enabled;
    crypto::secret_key skey;
    time_t last_info_query_time;
    std::string hashing_blob;
    uint64_t difficulty;
    uint64_t credits_per_hash_found;
    uint64_t height;
    std::string top_hash;
    uint32_t cookie;
    time_t last_hashing_blob_time;

    Entry(const std::string &mining_page = "", const std::string &location = "", const std::string &name = "");
  };

  explicit DB();

  void lock();
  void unlock();

  bool update(const std::string &mining_page, const std::string &location, const std::string &name, const uint64_t *payment, const uint64_t *balance, const std::string &hashing_blob, const uint64_t *difficulty, const uint64_t *credits_per_hash_found, const uint64_t *height, const std::string &top_hash, const uint32_t *cookie);

  std::string make_signature(const std::string &url);

  size_t get_num_entries();
  Entry get_entry(unsigned idx);
  bool has_entry(const std::string &mining_page);

  Q_INVOKABLE void deleteEntry(unsigned idx);
  Q_INVOKABLE void deleteAllEntries();
  Q_INVOKABLE void setTarget(unsigned idx, quint64 target);
  Q_INVOKABLE void setEnabled(unsigned idx, bool enabled);
  Q_INVOKABLE void setInfoQueried(unsigned idx);

  struct Locker
  {
    Locker(DB *db): db(db) { db->lock(); }
    ~Locker() { db->unlock(); }
  private:
    DB *db;
  };

signals:
  void refreshStarted();
  void refreshFinished();
  void modelChanged();
  void newSite(const QString &mining_page, const QString &name);

private:
  int find_mining_page(const std::string &mining_page);
  int find_url(const std::string &url);

private:
  std::deque<Entry> db;
  boost::recursive_mutex mutex;
};

#endif // DB_H
