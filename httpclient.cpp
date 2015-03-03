#include <chrono>
#include <folly/wangle/acceptor/Acceptor.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include "httpclient.h"

using proxygen::AsyncTimeoutSet;
using proxygen::HTTPUpstreamSession;
using proxygen::HTTPConnector;

namespace mv { namespace httpclient
{
void HTTPClient::ConnectCallback::connectSuccess(HTTPUpstreamSession* session)
{
  std::cout << "connect success" << std::endl;
  main_event_base_->terminateLoopSoon();
}

void HTTPClient::ConnectCallback::connectError(const apache::thrift::transport::TTransportException& ex)
{
  std::cout << "connect failed: " << ex.what() << std::endl;
  main_event_base_->terminateLoopSoon();
}

HTTPClient::HTTPClient(folly::EventBase* base)
  : main_event_base_(base)
{
}
  
HTTPClient::~HTTPClient()
{}

void HTTPClient::Start()
{

  // Global Event base manager that will be used to create all the worker
  auto timeout = std::chrono::milliseconds(100);
  std::cout << "Client timeout set to " << timeout.count() << std::endl;
  client_timeout_set_.reset(new AsyncTimeoutSet(main_event_base_, timeout));
}

// test GET
void HTTPClient::TestGet()
{
// transactionTimeouts_.reset(new AsyncTimeoutSet(
//                                  eventBase, accConfig_.transactionIdleTimeout));
  main_event_base_->runInEventBaseThread([this]() {
      auto cb = new ConnectCallback(main_event_base_);
      connector_.reset(new HTTPConnector(cb, client_timeout_set_.get()));
      folly::SocketAddress addr("www.baidu.com", 80, true);
      std::cout << "connecting to " << addr.getAddressStr() << std::endl;
      connector_->connect(main_event_base_, addr, std::chrono::milliseconds(100));
    });
}
}
}
