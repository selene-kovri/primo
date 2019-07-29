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

#ifndef NATIVE_MESSAGING_HANDLER_H
#define NATIVE_MESSAGING_HANDLER_H

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <string>

class Executor;

class NativeMessagingHandler: public QObject
{
  Q_OBJECT;

public:
  NativeMessagingHandler(Executor *executor);

  void run();

  Q_INVOKABLE void requestURL(const QString &url);

signals:
  void quit();

public slots:
  void onHashFound();

private:
  QSocketNotifier *notifier;
  QTimer *timer;
  Executor *executor;

private:
  void send(const std::string &s);

private slots:
  void readCommand();
  void tick();
};

#endif // NATIVE_MESSAGING_HANDLER_H
