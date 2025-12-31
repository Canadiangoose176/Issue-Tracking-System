#ifndef RUNNER_HPP_
#define RUNNER_HPP_

#include <memory>
#include <list>
#include <thread>

#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include <oatpp/network/Server.hpp>
#include <oatpp/web/server/HttpConnectionHandler.hpp>
#include <oatpp/web/server/HttpRouter.hpp>
#include <oatpp-swagger/Controller.hpp>
#include <oatpp/core/macro/component.hpp>

#include "Constants.hpp"
#include "controller/IssueApiController.hpp"
#include "SwaggerComponent.hpp"


class Runner {
 private:
  std::shared_ptr<oatpp::network::tcp::server::ConnectionProvider>
      m_tcpConnectionProvider;

 public:
  // Proper constructor for Runner, marked explicit for safety
  explicit Runner(const std::shared_ptr<
      oatpp::network::tcp::server::ConnectionProvider>& provider)
      : m_tcpConnectionProvider(provider) {}

  void run();
};

#endif  // RUNNER_HPP_
