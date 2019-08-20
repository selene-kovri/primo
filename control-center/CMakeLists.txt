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

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(primo_cc_sources
  src/main.cpp
  src/miner.cpp
  src/difficulty.cpp
  src/native-messaging-handler.cpp
  src/event-filter.cpp
  src/executor.cpp
  src/db.cpp
  src/signature.cpp
  src/model.cpp

  src/cJSON/cJSON.c
  src/cJSON/cJSON_Utils.c

 
  src/crypto/crypto.cpp

  src/crypto/CryptonightR_JIT.c
  src/crypto/crypto-ops-data.c
  src/crypto/hash-extra-groestl.c
  src/crypto/hmac-keccak.c
  src/crypto/random.c
  src/crypto/aesb.c	
  src/crypto/crypto-ops.c	
  src/crypto/hash-extra-jh.c
  src/crypto/jh.c		
  src/crypto/skein.c
  src/crypto/blake256.c	
  src/crypto/groestl.c	
  src/crypto/hash-extra-skein.c
  src/crypto/keccak.c	
  src/crypto/slow-hash.c
  src/crypto/chacha.c	
  src/crypto/hash-extra-blake.c
  src/crypto/hash.c		
  src/crypto/oaes_lib.c	
  src/crypto/tree-hash.c

  src/crypto/CryptonightR_template.S

  src/epee/src/hex.cpp
  src/epee/src/memwipe.c
  src/epee/src/mlocker.cpp
  src/epee/src/mlog.cpp
  src/epee/src/string_tools.cpp
  src/epee/src/wipeable_string.cpp

  src/easylogging++/easylogging++.cc

  primo-control-center.qrc
)
set_property(SOURCE src/crypto/CryptonightR_template.S PROPERTY LANGUAGE C)

set(primo_cc_headers
  src/miner.h
  src/difficulty.h
  src/native-messaging-handler.h
  src/event-filter.h
  src/executor.hsrc/db.h
  src/signature.hsrc/model.h

  src/cJSON/cJSON.h
  src/cJSON/cJSON_Utils.h

  src/crypto/CryptonightR_JIT.h
  src/crypto/crypto.h
  src/crypto/hash.h
  src/crypto/oaes_config.h
  src/crypto/variant2_int_sqrt.h
  src/crypto/CryptonightR_template.h
  src/crypto/generic-ops.h
  src/crypto/hmac-keccak.h
  src/crypto/oaes_lib.h
  src/crypto/variant4_random_math.h
  src/crypto/blake256.h
  src/crypto/groestl.h
  src/crypto/initializer.h
  src/crypto/random.h
  src/crypto/chacha.h
  src/crypto/groestl_tables.h
  src/crypto/jh.h
  src/crypto/skein.h
  src/crypto/crypto-ops.h
  src/crypto/hash-ops.h
  src/crypto/keccak.h
  src/crypto/skein_port.h

  src/epee/include/fnv1.h
  src/epee/include/memwipe.h
  src/epee/include/span.h
  src/epee/include/wipeable_string.h
  src/epee/include/hex.h
  src/epee/include/misc_log_ex.h
  src/epee/include/string_tools.h
  src/epee/include/include_base_utils.h
  src/epee/include/misc_os_dependent.h
  src/epee/include/syncobj.h
  src/epee/include/int-util.h
  src/epee/include/mlocker.h
  src/epee/include/warnings.h

  src/epee/include/storages/parserse_base_utils.h

  src/easylogging++/easylogging++.h
  src/easylogging++/ea_config.h
)

find_package(Qt5Qml REQUIRED)
find_package(Qt5Quick REQUIRED)

include_directories(src/epee/include)
include_directories(src/easylogging++)
include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -W -O2")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DAUTO_INITIALIZE_EASYLOGGINGPP")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

if (CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
  message("Enabling hardware instructions on ${CMAKE_SYSTEM_PROCESSOR}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -maes")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -maes")
endif()

add_executable(primo-control-center
  ${primo_cc_sources}
)

target_link_libraries(primo-control-center
  boost_filesystem
  boost_thread
  boost_chrono
  boost_system
)

qt5_use_modules(primo-control-center Qml Quick)

