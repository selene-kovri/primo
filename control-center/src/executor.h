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

#ifndef EXECUTOR_H
#define EXECUTOR_H

#include <QObject>
#include <string>

class QGuiApplication;
class DB;
class Miner;

class Executor: public QObject
{
  Q_OBJECT

public:
  Executor(QGuiApplication *gui, DB *db, Miner *miner);
  std::string execute(const std::string &command);
  std::string tick();
  std::string submit_nonce();

public slots:
  void onHashFound(const std::string &mining_page, uint32_t nonce, const std::string &top_hash, uint32_t cookie);

signals:
  void hashFound();

private:
  std::string cmd_site(const std::string &id, const std::string &mining_page, const std::string &location, const std::string &name, const uint64_t *payment, const uint64_t *credits, const std::string &hashing_blob, const uint64_t *difficulty, const uint64_t *credits_per_hash_found, const uint64_t *height, const std::string &top_hash, const uint32_t *cookie);
  std::string cmd_sign(const std::string &id, const std::string &mining_page);
  std::string cmd_forget_all(const std::string &id);
  std::string cmd_summon(const std::string &id);

private:
  QGuiApplication *gui;
  DB *db;
  Miner *miner;

  std::string found_mining_page;
  std::string found_top_hash;
  uint32_t found_nonce;
  uint32_t found_cookie;
};

#endif // EXECUTOR_H
