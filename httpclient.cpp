#include <chrono>
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
  connector_.reset(new HTTPConnector(this, client_timeout_set_.get()));
  host_ = host;
  main_event_base_->runInEventBaseThread([this, timeout]() {
      folly::SocketAddress addr;
      addr.setFromHostPort(host_);
      std::cout << "connecting to " << addr.getAddressStr() 
                << " with timeout " << timeout.count() << "ms" << std::endl;
      connector_->connect(main_event_base_, addr, timeout);
    });
}

void HTTPClient::connectSuccess(HTTPUpstreamSession* session)
{
  std::cout << "connect success" << std::endl;

  session->setMaxConcurrentOutgoingStreams(100);

  std::cout << "max out going streams: " 
            << session->getMaxConcurrentOutgoingStreams() << std::endl;
  
  for (int i = 0; i < 10; i++) {
    HTTPTransaction* txn = session->newTransaction(new ResponseHandler());
    if (txn) {
      HTTPMessage req_msg;
      req_msg.setURL("http://" + host_);
      req_msg.rawSetMethod("GET");
      std::cout << "send request message:" << std::endl;
      req_msg.getHeaders().forEach([&] (const std::string& header, const std::string& val) {
          std::cout << header << ": " << val << std::endl;
        });
      txn->sendHeaders(req_msg);
      txn->sendEOM();
    }
    else {
      std::cout << "support more transaction: " 
                << session->supportsMoreTransactions() << std::endl;
    }
  }
}

void HTTPClient::connectError(const apache::thrift::transport::TTransportException& ex)
{
  std::cout << "connect failed: " << ex.what() << std::endl;
  main_event_base_->terminateLoopSoon();
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
