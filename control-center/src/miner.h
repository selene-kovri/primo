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

#ifndef MINER_H
#define MINER_H

#include <boost/thread.hpp>
#include <boost/circular_buffer.hpp>
#include <string>
#include <vector>
#include <QObject>
#include <QString>

class Miner: public QObject
{
  Q_OBJECT
  Q_PROPERTY(float hashRate READ hashRate NOTIFY hashRateChanged)
  Q_PROPERTY(bool paused READ paused WRITE pause NOTIFY pausedChanged)
  Q_PROPERTY(QString miningPage READ miningPage NOTIFY miningPageChanged)
  Q_PROPERTY(QString miningName READ miningName NOTIFY nameChanged)

public:
  explicit Miner(QObject *parent = nullptr);
  virtual ~Miner();

  void set_hashing_blob(const std::string &new_hashing_blob, uint64_t new_difficulty, uint64_t new_height, const std::string &new_top_hash, uint32_t new_cookie, const std::string &new_mining_page, const std::string &new_mining_name, bool keep_mining_page= false);
  void stop();
  void pause(bool p = true);
  void update(const std::string &mining_page, const std::string &hashing_blob, uint64_t difficulty, uint64_t height, const std::string &top_hash, uint32_t cookie);
  std::string get_mining_page();

  float hashRate();
  bool paused();
  QString miningPage();
  QString miningName();

signals:
  void hashFound(const std::string &mining_page, uint32_t nonce, const std::string &top_hash, uint32_t cookie);
  void miningPageChanged(const QString &mining_page);
  void nameChanged(const QString &mining_name);
  void hashRateChanged(float hash_rate);
  void pausedChanged();

private:
  void mining_thread();
  float get_hash_rate_average() const;

private:
  std::string hashing_blob;
  std::vector<uint8_t> raw_hashing_blob;
  uint64_t difficulty;
  uint64_t height;
  std::string top_hash;
  std::string mining_page;
  std::string mining_name;
  uint32_t cookie;

  bool running;
  boost::mutex mutex;
  boost::condition_variable cond;
  boost::thread thread;
  uint64_t n_hashes;
  boost::posix_time::ptime hash_rate_window_start_time;
  uint64_t hash_rate_window_start_hashes;
  bool is_paused;
  boost::circular_buffer<float> hash_rate_history;
};

#endif // MINER_H
