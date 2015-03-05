#pragma once

#include <memory>
#include <deque>
#include <vector>
#include <map>
#include <thread>
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

class HTTPClientConnector;

class HTTPClient
{
public:
  // Create a new HTTPClient
  explicit HTTPClient(folly::EventBase* base);
  ~HTTPClient();

  // connect to host:port
  void Connect(const std::string& host, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));

  void Start();

private:
  friend class HTTPClientConnector;
  using ConnectorPtr = std::unique_ptr<HTTPClientConnector>;
  HTTPClientConnector* GetConnector();
  void ReleaseConnector(HTTPClientConnector* connector);

  void AddSession(int32_t id, int32_t session_id,
                  proxygen::HTTPUpstreamSession* session);

  int32_t connector_count_{0};
  std::mutex cm_;
  std::deque<ConnectorPtr> connectors_;
  folly::EventBase* main_event_base_{nullptr};
  proxygen::AsyncTimeoutSet::UniquePtr client_timeout_set_;
  std::string host_;
  std::vector<std::map<int32_t, proxygen::HTTPUpstreamSession*> > sessions_;
};

class HTTPClientConnector : public proxygen::HTTPConnector::Callback
{
public:
  HTTPClientConnector(int32_t id, HTTPClient* client);
  ~HTTPClientConnector();

  HTTPClientConnector(HTTPClientConnector&& rhs);
  HTTPClientConnector& operator=(HTTPClientConnector&& rhs);

  void Connect(folly::EventBase*, proxygen::AsyncTimeoutSet*,
               const std::string& host, std::chrono::milliseconds timeout);

  int32_t GetID() const { return id_; }
  bool isBusy() const { return connector_ && connector_->isBusy(); }

  // connect callback interface
  virtual void connectSuccess(proxygen::HTTPUpstreamSession* session);
  virtual void connectError(
    const apache::thrift::transport::TTransportException& ex);

private:
  int32_t id_;
  int32_t count_{0};
  HTTPClient* client_{nullptr};
  std::unique_ptr<proxygen::HTTPConnector> connector_;
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
