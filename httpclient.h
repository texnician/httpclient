
#pragma once

#include <memory>
#include <proxygen/lib/http/HTTPConnector.h>

namespace mv { namespace httpclient {
using proxygen::HTTPConnector;

class HTTPClient
{
public:
  // Create a new HTTPClient
  HTTPClient();
  ~HTTPClient();

  // test GET
  void TestGet();

private:
  std::shared_ptr<HTTPConnector> connector_;
};
} // httpclient 
} // mv
