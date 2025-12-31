// SPDX-License-Identifier: MIT
// Entry point helper that wires the HTTP router, controllers, and server.
#include "Runner.hpp"

#include "oatpp/web/server/interceptor/AllowCorsGlobal.hpp"

void Runner::run() {
  // Pull shared components configured in AppComponent/SwaggerComponent.
  OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>,
                  objectMapper);
  OATPP_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>, docInfo,
                  Qualifiers::SERVICE_ISSUE);
  OATPP_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>, resources,
                  Qualifiers::SERVICE_ISSUE);

  auto router = oatpp::web::server::HttpRouter::createShared();

  // Register REST API endpoints.
  auto issueController =
      std::make_shared<IssueApiController>(objectMapper);
  router->addController(issueController);

  // Register Swagger UI/JSON docs.
  auto swaggerController =
      oatpp::swagger::Controller::createShared(issueController->getEndpoints(),
                                               docInfo, resources);
  router->addController(swaggerController);

  auto connectionHandler =
      oatpp::web::server::HttpConnectionHandler::createShared(router);
  // Allow browser requests from other origins (dev UI on different port).
  // Explicitly include PATCH/DELETE so inline edits from the design UI succeed.
  connectionHandler->addResponseInterceptor(
      std::make_shared<oatpp::web::server::interceptor::AllowCorsGlobal>(
          "*", "GET, POST, PATCH, DELETE, OPTIONS"));
  connectionHandler->addRequestInterceptor(
      std::make_shared<oatpp::web::server::interceptor::AllowOptionsGlobal>());

  oatpp::network::Server server(m_tcpConnectionProvider, connectionHandler);
  server.run();
}
