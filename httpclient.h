#pragma once

#include <memory>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <proxygen/lib/utils/AsyncTimeoutSet.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPTransaction.h>

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

class HTTPClient : public proxygen::HTTPConnector::Callback
{
public:
  // Create a new HTTPClient
  explicit HTTPClient(folly::EventBase* base);
  ~HTTPClient();

  // connect to host:port
  void Connect(const std::string& host, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

  void Start();

  // connect callback interface
  virtual void connectSuccess(proxygen::HTTPUpstreamSession* session);
  virtual void connectError(
      const apache::thrift::transport::TTransportException& ex);

private:
  std::unique_ptr<proxygen::HTTPConnector> connector_;
  folly::EventBase* main_event_base_{nullptr};
  
  proxygen::AsyncTimeoutSet::UniquePtr client_timeout_set_;
  std::string host_;
};

class ResponseHandler : public proxygen::HTTPTransactionHandler
{
public:
  ResponseHandler();
  ~ResponseHandler();
  void setTransaction(proxygen::HTTPTransaction* txn) noexcept override;
  void detachTransaction() noexcept override;
  void onHeadersComplete(std::unique_ptr<proxygen::HTTPMessage> msg) noexcept override;
  void onBody(std::unique_ptr<folly::IOBuf> chain) noexcept override;
  void onChunkHeader(size_t length) noexcept override;
  void onChunkComplete() noexcept override;
  void onTrailers(std::unique_ptr<proxygen::HTTPHeaders> trailers) noexcept override;
  void onEOM() noexcept override;
  void onUpgrade(proxygen::UpgradeProtocol protocol) noexcept override;
  void onError(const proxygen::HTTPException& error) noexcept override;
  void onEgressPaused() noexcept override;
  void onEgressResumed() noexcept override;

  // Helper method
  void setError(proxygen::ProxygenError err) noexcept;
private:
  proxygen::HTTPTransaction* txn_{nullptr};
  proxygen::ProxygenError err_{proxygen::kErrorNone};
};

} // httpclient 
} // mv
