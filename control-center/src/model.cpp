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

#include <sstream>
#include <QHash>
#include "db.h"
#include "model.h"

static std::string get_minutes_per_request(uint64_t payment, uint64_t difficulty, uint64_t credits_per_hash_found, float hash_rate)
{
  if (credits_per_hash_found == 0 || difficulty == 0)
    return "-";
  if (hash_rate < 0.01f)
    return "?";
  std::stringstream str;
  const float minutes_per_request = payment * difficulty / (float)credits_per_hash_found / hash_rate / 60;
  str.precision(minutes_per_request >= 100 ? 0 : minutes_per_request >=10 ? 1 : 2);
  str << std::fixed << minutes_per_request;
  return str.str();
}

Model::Model(QObject *parent, DB *db):
  QAbstractListModel(parent),
  db(db),
  hash_rate(0.0f)
{
  connect(db, SIGNAL(refreshStarted()), this, SLOT(startReset()));
  connect(db, SIGNAL(refreshFinished()), this, SLOT(endReset()));
  connect(db, SIGNAL(modelChanged()), this, SLOT(onModelChanged()));
}

void Model::onModelChanged()
{
  // dataChanged gets ignored, but start/end don't...
  //emit dataChanged();
  startReset();
  endReset();

emit numRowsChanged();
}

void Model::startReset()
{
  beginResetModel();
}

void Model::endReset()
{
  endResetModel();
}

void Model::onHashRateChanged(float new_hash_rate)
{
  hash_rate = new_hash_rate;
}

int Model::rowCount(const QModelIndex &) const
{
  return db->get_num_entries();
}

QVariant Model::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
    return QVariant();
  const auto row = index.row();
  if (row < 0 || (size_t)row >= db->get_num_entries())
    return QVariant();

  const DB::Entry entry = db->get_entry(row);
  switch (role)
  {
    case RoleMiningPage: return QString::fromStdString(entry.mining_page);
    case RoleName: return QString::fromStdString(entry.name);
    case RoleNameOrPage: return QString::fromStdString(entry.name.empty() ? entry.mining_page : entry.name);
    case RoleBalance: return QVariant::fromValue(entry.balance);
    case RoleBalanceString: return QString::number(entry.balance);
    case RoleTarget: return QVariant::fromValue(entry.target);
    case RoleTargetString: return QString::number(entry.target);
    case RoleEnabled: return entry.enabled;
    case RoleDifficulty: return QString::number(entry.difficulty);
    case RoleCreditsPerHashFound: return QString::number(entry.credits_per_hash_found);
    case RoleMinutesPerRequestString: return QString::fromStdString(get_minutes_per_request(entry.payment, entry.difficulty, entry.credits_per_hash_found, hash_rate));
    default: return QVariant();
  }
}

QHash<int, QByteArray> Model::roleNames() const
{
  QHash<int, QByteArray> names = QAbstractListModel::roleNames();
  names.insert(RoleMiningPage, "mining_page");
  names.insert(RoleName, "name");
  names.insert(RoleNameOrPage, "name_or_page");
  names.insert(RoleBalance, "balance");
  names.insert(RoleBalanceString, "balance_string");
  names.insert(RoleTarget, "target");
  names.insert(RoleTargetString, "target_string");
  names.insert(RoleEnabled, "enabled");
  names.insert(RoleDifficulty, "difficulty");
  names.insert(RoleCreditsPerHashFound, "credits_per_hash_found");
  names.insert(RoleMinutesPerRequestString, "minutes_per_request_string");
  return names;
}
