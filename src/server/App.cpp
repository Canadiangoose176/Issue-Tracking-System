#include "AppComponent.hpp"
#include "Runner.hpp"
#include "SwaggerComponent.hpp"

#include <oatpp/core/macro/component.hpp>
#include <oatpp/network/tcp/server/ConnectionProvider.hpp>

void run() {
  // Register Oat++ components
  AppComponent components;
  SwaggerComponent swaggerComponents;
  (void)components;
  (void)swaggerComponents;

  OATPP_COMPONENT(
    std::shared_ptr<oatpp::network::tcp::server::ConnectionProvider>,
                  connectionProvider);

  // Create runner with provider
  Runner runner(connectionProvider);

  // Run server
  runner.run();
}

int main(int argc, const char* argv[]) {
  oatpp::base::Environment::init();
  run();
  oatpp::base::Environment::destroy();
  return 0;
}
