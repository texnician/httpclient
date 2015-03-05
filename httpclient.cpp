#include <stdio.h>
#include <chrono>
#include <cassert>
#include <folly/wangle/acceptor/Acceptor.h>
#include <proxygen/lib/http/HTTPConnector.h>
#include <proxygen/lib/http/session/HTTPUpstreamSession.h>
#include "httpclient.h"

using namespace proxygen;

namespace mv { namespace httpclient
{
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

void HTTPClient::Connect(const std::string& host, std::chrono::milliseconds timeout)
{
  // connector_.reset(new HTTPConnector(this, client_timeout_set_.get()));
  host_ = host;
  for (int i = 0; i < 10; ++i) {
    auto connector = GetConnector();
    connector->Connect(main_event_base_, client_timeout_set_.get(), host_, timeout);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void HTTPClient::AddSession(int32_t connector_id, int32_t session_id,
                            HTTPUpstreamSession* session)
{
  printf("connector %d add new session %d to client\n", connector_id, session_id);
  // TODO(tyg) need lock
  assert(connector_id < sessions_.size());
  sessions_[connector_id][session_id] = session;
}

HTTPClientConnector* HTTPClient::GetConnector()
{
  // TODO(tyg) need lock
  std::lock_guard<std::mutex> lock(cm_);
  if (connectors_.empty()) {
    auto new_connector = new HTTPClientConnector(connector_count_, this);
    sessions_.push_back(std::map<int32_t, HTTPUpstreamSession*>());
    ++connector_count_;
    return new_connector;
  }
  else {
    auto connector = std::move(connectors_.front());
    connectors_.pop_front();
    assert(!connector->isBusy());
    return connector.release();
  }
}

void HTTPClient::ReleaseConnector(HTTPClientConnector* connector)
{
  // TODO(tyg) need lock
  assert(!connector->isBusy());
  std::lock_guard<std::mutex> lock(cm_);
  connectors_.emplace_back(ConnectorPtr(connector));
}

HTTPClientConnector::HTTPClientConnector(int32_t id, HTTPClient* client)
  : id_(id), client_(client)
{
}

HTTPClientConnector::HTTPClientConnector(HTTPClientConnector&& rhs)
  : id_(rhs.id_), count_(rhs.count_), client_(rhs.client_),
    connector_(std::move(rhs.connector_))
{
}

HTTPClientConnector& HTTPClientConnector::operator=(HTTPClientConnector&& rhs)
{
  id_ = rhs.id_;
  count_ = rhs.count_;
  client_ = rhs.client_;
  connector_ = std::move(rhs.connector_);
  return *this;
}

HTTPClientConnector::~HTTPClientConnector()
{}

void HTTPClientConnector::Connect(folly::EventBase* base, 
                                  AsyncTimeoutSet* client_timeout_set,
                                  const std::string& host,
                                  std::chrono::milliseconds timeout)
{
  base->runInEventBaseThread([this, base, client_timeout_set, &host, timeout]() {
      folly::SocketAddress addr;
      addr.setFromHostPort(host);
      std::cout << "connecting to " << addr.getAddressStr() 
                << " with timeout " << timeout.count() << "ms" << std::endl;
      if (!connector_) {
         connector_.reset(new HTTPConnector(this, client_timeout_set));
      }
      connector_->connect(base, addr, timeout);
    });
}

void HTTPClientConnector::connectSuccess(HTTPUpstreamSession* session)
{
  printf("connector %d connect success, session id is %d\n", id_, count_);

  std::cout << "max out going streams: " 
            << session->getMaxConcurrentOutgoingStreams() << std::endl;
  
  client_->AddSession(id_, count_++, session);
  printf("release connector %d to client\n", id_);
  client_->ReleaseConnector(this);
  
  // for (int i = 0; i < 10; i++) {
  //   HTTPTransaction* txn = session->newTransaction(new ResponseHandler());
  //   if (txn) {
  //     HTTPMessage req_msg;
  //     req_msg.setURL("http://" + host_);
  //     req_msg.rawSetMethod("GET");
  //     std::cout << "send request message:" << std::endl;
  //     req_msg.getHeaders().forEach([&] (const std::string& header, const std::string& val) {
  //         std::cout << header << ": " << val << std::endl;
  //       });
  //     txn->sendHeaders(req_msg);
  //     txn->sendEOM();
  //   }
  //   else {
  //     std::cout << "support more transaction: " 
  //               << session->supportsMoreTransactions() << std::endl;
  //   }
  // }
}

void HTTPClientConnector::connectError(const apache::thrift::transport::TTransportException& ex)
{
  std::cout << "connect failed: " << ex.what() << std::endl;
  printf("release failed connector %d to client\n", id_);
  client_->ReleaseConnector(this);
  // main_event_base_->terminateLoopSoon();
}

ResponseHandler::ResponseHandler()
{}

ResponseHandler::~ResponseHandler()
{}

void ResponseHandler::setTransaction(HTTPTransaction* txn) noexcept
{
  txn_ = txn;
}

void ResponseHandler::detachTransaction() noexcept
{
  std::cout << "response complete, detach transaction" << std::endl;
  delete this;
}

void ResponseHandler::onHeadersComplete(std::unique_ptr<HTTPMessage> msg) noexcept
{
  std::cout << "response header complete" << std::endl;
  msg->getHeaders().forEach([&] (const std::string& header, const std::string& val) {
      std::cout << header << ": " << val << std::endl;
    });
}

void ResponseHandler::onBody(std::unique_ptr<folly::IOBuf> chain) noexcept
{}

void ResponseHandler::onChunkHeader(size_t length) noexcept
{}

void ResponseHandler::onChunkComplete() noexcept
{}

void ResponseHandler::onTrailers(std::unique_ptr<HTTPHeaders> trailers) noexcept
{}

void ResponseHandler::onEOM() noexcept
{
  std::cout << "response EOM" << std::endl;
}

void ResponseHandler::onUpgrade(UpgradeProtocol protocol) noexcept
{}

void ResponseHandler::onError(const HTTPException& error) noexcept
{
  std::cout << "response handler error: " << error.what() << std::endl;
  
  if (err_ != kErrorNone) {
    // we have already handled an error and upstream would have been deleted
    return;
  }

  if (error.getProxygenError() == kErrorTimeout) {
    std::cout << "response timeout" << std::endl;
  } else if (error.getDirection() == HTTPException::Direction::INGRESS) {
    setError(kErrorRead);
    std::cout << "response read error" << std::endl;
  } else {
    std::cout << "response write error" << std::endl;
    setError(kErrorWrite);
  }
  // Wait for detachTransaction to clean up
}

void ResponseHandler::onEgressPaused() noexcept
{}

void ResponseHandler::onEgressResumed() noexcept
{}

void ResponseHandler::setError(ProxygenError err) noexcept
{
  err_ = err;
}
}
}
