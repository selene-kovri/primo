#  Primo - Privacy with Monero
#
#  Copyright (C) 2019 selene
#
#  This file is part of Primo.
#
#  Primo is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Primo is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Primo.  If not, see <https://www.gnu.org/licenses/>.

TEMPLATE     = app

QT          += qml quick

CONFIG      += qml_debug debug

HEADERS     += $$PWD/src/miner.h $$PWD/src/difficulty.h \
               $$PWD/src/native-messaging-handler.h $$PWD/src/event-filter.h \
               $$PWD/src/executor.h $$PWD/src/db.h \
               $$PWD/src/signature.h \
               $$PWD/src/model.h \
               $$PWD/src/cJSON/cJSON.h $$PWD/src/cJSON/cJSON_Utils.h \
               $$PWD/src/crypto/*.h \
               $$PWD/src/epee/include/*.h $$PWD/src/epee/include/storages/*.h \
               $$PWD/src/easylogging++/easylogging++.h $$PWD/src/easylogging++/ea_config.h

SOURCES     += $$PWD/src/main.cpp \
               $$PWD/src/miner.cpp $$PWD/src/difficulty.cpp \
               $$PWD/src/native-messaging-handler.cpp $$PWD/src/event-filter.cpp \
               $$PWD/src/executor.cpp $$PWD/src/db.cpp \
               $$PWD/src/signature.cpp \
               $$PWD/src/model.cpp \
               $$PWD/src/cJSON/cJSON.c $$PWD/src/cJSON/cJSON_Utils.c \
               $$PWD/src/crypto/*.cpp $$PWD/src/crypto/*.c $$PWD/src/crypto/*.S \
               $$PWD/src/epee/src/*.cpp $$PWD/src/epee/src/*.c \
               $$PWD/src/easylogging++/easylogging++.cc

# that's probably wrong
linux-g++:QMAKE_TARGET.arch = $$QMAKE_HOST.arch
linux-g++-32:QMAKE_TARGET.arch = x86
linux-g++-64:QMAKE_TARGET.arch = x86_64

contains(QMAKE_TARGET.arch, x86_64) {
  message("x86_64, enabling AES")
  MAES=-maes
} else {
  message("Not x86_64, no AES")
}

QMAKE_CXXFLAGS += -Wall -W -O2 -I $$PWD/src/epee/include -I $$PWD/src -I $$PWD/src/easylogging++ -DAUTO_INITIALIZE_EASYLOGGINGPP
QMAKE_CFLAGS += -Wall -W -O2 -I $$PWD/src/epee/include -I $$PWD/src $$MAES
LIBS += -lboost_filesystem -lboost_thread -lboost_system

RESOURCES   += primo-control-center.qrc

INSTALLS    += target

OTHER_FILES  += \
                primo-control-center.qml \
                content/*.png
