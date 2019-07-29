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

#include <unistd.h>
#include <memory>
#include "executor.h"
#include "native-messaging-handler.h"

#include <errno.h>
#include <string.h>

#define TICK_MS 1000

NativeMessagingHandler::NativeMessagingHandler(Executor *executor):
  executor(executor)
{
  notifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read, this);
  timer = new QTimer(this);
  timer->setInterval(TICK_MS);
  timer->start(TICK_MS);
  connect(executor, SIGNAL(hashFound()), this, SLOT(onHashFound()));
}

void NativeMessagingHandler::run()
{
  connect(notifier, SIGNAL(activated(int)), this, SLOT(readCommand()));
  connect(timer, SIGNAL(timeout()), this, SLOT(tick()));
}

void NativeMessagingHandler::requestURL(const QString &qurl)
{
  const std::string url = qurl.toStdString();
  send("{\"cmd\":\"url\", \"url\":\""+url+"\"}");
}

void NativeMessagingHandler::send(const std::string &s)
{
  //fprintf(stderr, "Sending: %s\n", s.c_str());
  uint32_t len = s.size();
  write(1, &len, sizeof(len));
  write(1, s.c_str(), len);
}

void NativeMessagingHandler::readCommand()
{
  ssize_t bytes;
  uint32_t len;

  errno=0;
  bytes = read(0, &len, sizeof(len));
  if (bytes != sizeof(len))
  {
    fprintf(stderr, "Error reading message length: %zd, %s\n", bytes, strerror(errno));
    return;
  }
  if (len > 0)
  {
    std::unique_ptr<char[]> buffer(new char[len+1]);
    bytes = read(0, buffer.get(), len);
    if (bytes != len)
    {
      fprintf(stderr, "Error reading message payload\n");
      return;
    }
    buffer.get()[len] = 0;

    std::string reply = executor->execute(buffer.get());
    send(reply);
  }
}

void NativeMessagingHandler::tick()
{
  while (1)
  {
    std::string cmd = executor->tick();
    if (cmd.empty())
      break;
    send(cmd);
  }
}

void NativeMessagingHandler::onHashFound()
{
  std::string cmd = executor->submit_nonce();
  send(cmd);
}
