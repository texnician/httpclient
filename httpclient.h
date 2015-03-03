#pragma once

#include <memory>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <proxygen/lib/utils/AsyncTimeoutSet.h>
#include <proxygen/lib/http/HTTPConnector.h>

namespace folly
{
class EventBase;
}

namespace proxygen
{
class AsyncTimeoutSet;
class HTTPUpstreamSession;
}

namespace mv { namespace httpclient {

class HTTPClient
{
public:
  class ConnectCallback : public proxygen::HTTPConnector::Callback
  {
  public:
    explicit ConnectCallback(folly::EventBase* ev)
      : main_event_base_(ev)
      {}
    virtual void connectSuccess(proxygen::HTTPUpstreamSession* session);
    virtual void connectError(
      const apache::thrift::transport::TTransportException& ex);
  private:
    folly::EventBase* main_event_base_;
  };
  // Create a new HTTPClient
  explicit HTTPClient(folly::EventBase* base);
  ~HTTPClient();

  // test GET
  void TestGet();

  void Start();

private:
  std::unique_ptr<proxygen::HTTPConnector> connector_;
  folly::EventBase* main_event_base_{nullptr};
  
  proxygen::AsyncTimeoutSet::UniquePtr client_timeout_set_;
};
} // httpclient 
} // mv
