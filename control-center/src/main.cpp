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

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include "native-messaging-handler.h"
#include "event-filter.h"
#include "miner.h"
#include "executor.h"
#include "db.h"
#include "model.h"

Q_DECLARE_METATYPE(uint32_t)
Q_DECLARE_METATYPE(std::string)

int main(int argc, char **argv)
{
  const int is_native_messaging = argc == 3 && strstr(argv[1], "primo.control.center.json") && strstr(argv[2], "primo");

  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
  QCoreApplication::setOrganizationName("None");

  QQuickWindow::setDefaultAlphaBuffer(true);

  QGuiApplication gui(argc, argv);
  qmlRegisterType<Miner>("Miner", 1, 0, "Miner");
  qmlRegisterType<Model>("Model", 1, 0, "Model");
  qRegisterMetaType<uint32_t>("uint32_t");
  qRegisterMetaType<std::string>("std::string");

  DB db;
  Model model(&db, &db);
  Miner miner(&gui);
  Executor executor(&gui, &db, &miner);
  NativeMessagingHandler handler(&executor);
  handler.run();

  QObject::connect(&miner, SIGNAL(hashRateChanged(float)), &model, SLOT(onHashRateChanged(float)));

  QQmlApplicationEngine engine;
  engine.rootContext()->setContextProperty("mainApp", &gui);
  engine.rootContext()->setContextProperty("db", &db);
  engine.rootContext()->setContextProperty("handler", &handler);
  engine.rootContext()->setContextProperty("miner", &miner);
  engine.rootContext()->setContextProperty("dbmodel", &model);
  engine.rootContext()->setContextProperty("is_native_messaging", is_native_messaging);

  gui.installEventFilter(new EventFilter([&gui](QObject *, QEvent *ev){
    if (ev->type() != QEvent::Close)
      return false;
    QList<QWindow*> l = gui.topLevelWindows();
    if (!l.isEmpty())
    {
      QWindow *window = l.front();
      if (window)
      {
        window->hide();
      }
    }
    return true;
  }));

  engine.load(QStringLiteral("qrc:///primo-control-center.qml"));
  return gui.exec();
}
