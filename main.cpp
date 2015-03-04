#include <iostream>
#include <thread>
#include <folly/ThreadName.h>
#include <folly/io/async/EventBaseManager.h>
#include <folly/io/async/EventBase.h>
#include "httpclient.h"

using namespace mv::httpclient;

int main(int argc, char* argv[])
{
  std::cout << "Hardware concurrency: " 
            << std::thread::hardware_concurrency() << std::endl;

// threads eventbases
  auto manager = folly::EventBaseManager::get();
  folly::EventBase* main_event_base = manager->getEventBase();
  std::thread client_event_thread([main_event_base](){
      folly::setThreadName("client-event-base");
      std::cout << "Run http client EventBase in thread " << std::this_thread::get_id() << std::endl;
      std::cout << "Client EventBase loop for ever" << std::endl;
      main_event_base->loopForever();
      std::cout << "Client EventBase exit" << std::endl;
    });

  HTTPClient client(main_event_base);
  client.Start();

  if (argc >= 2) {
    std::string host = argv[1];
    int timeout = argc > 2 ? atoi(argv[2]) : 100;
    client.Connect(host, std::chrono::milliseconds(timeout));
  }

  client_event_thread.join();
  std::cout << "thread joined" << std::endl;
}
