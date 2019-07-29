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

#ifndef SITE_MODEL_H
#define SITE_MODEL_H

#include <QObject>
#include <QAbstractListModel>

class DB;

class Model: public QAbstractListModel
{
  Q_OBJECT

  Q_PROPERTY(int numRows READ numRows NOTIFY numRowsChanged)

public:
  enum SiteRole
  {
    RoleMiningPage = Qt::UserRole + 1,
    RoleName,
    RoleNameOrPage,
    RoleBalance,
    RoleBalanceString,
    RoleTarget,
    RoleTargetString,
    RoleEnabled,
    RoleDifficulty,
    RoleCreditsPerHashFound,
    RoleMinutesPerRequestString
  };
  Q_ENUM(SiteRole)

  Model(QObject *parent = NULL, DB *db = NULL);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &parent, int role = Qt::DisplayRole) const override;
  virtual QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE int numRows() const { return rowCount(); }

public slots:
  void startReset();
  void endReset();
  void onModelChanged();
  void onHashRateChanged(float hash_rate);

signals:
  void dataChanged();
  void numRowsChanged();

private:
  DB *db;
  float hash_rate;
};

#endif // SITE_MODEL_H
