#ifndef APP_COMPONENT_HPP_
#define APP_COMPONENT_HPP_

#include <memory>

#include "Constants.hpp"

#include "SwaggerComponent.hpp"

#include "oatpp/web/server/HttpRouter.hpp"

#include "oatpp/network/virtual_/server/ConnectionProvider.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"

#include "oatpp/parser/json/mapping/ObjectMapper.hpp"

#include "oatpp/core/macro/component.hpp"


class AppComponent {
 public:
  OATPP_CREATE_COMPONENT(std::shared_ptr<
    oatpp::network::tcp::server::ConnectionProvider>,
    serverConnectionProvider)([] {
      return oatpp::network::tcp::server::ConnectionProvider::
          createShared({"0.0.0.0", 8600});
    }());

  OATPP_CREATE_COMPONENT(std::shared_ptr<
    oatpp::data::mapping::ObjectMapper>,
    apiObjectMapper)([] {
      return oatpp::parser::json::mapping::ObjectMapper::
          createShared();
    }());
};

#endif
