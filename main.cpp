#include <iostream>
#include <thread>
#include "httpclient.h"

using namespace mv::httpclient;

int main(int argc, char** argv)
{
  std::cout << "Hardware concurrency: " 
            << std::thread::hardware_concurrency() << std::endl;
  std::thread t([](){
      std::cout << "Run http client in thread " << std::this_thread::get_id() << std::endl;
      HTTPClient client;
      client.TestGet();
    });
  t.join();
  std::cout << "thread joined" << std::endl;
}
