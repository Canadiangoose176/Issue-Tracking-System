
#ifndef issue_SwaggerComponent_hpp
#define issue_SwaggerComponent_hpp

#include <memory>
#include "Constants.hpp"

#include "oatpp-swagger/Model.hpp"
#include "oatpp-swagger/Resources.hpp"
#include "oatpp/core/macro/component.hpp"

class SwaggerComponent {
 public:
  /**
   *  General API docs info
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>,
    swaggerDocumentInfo)(Qualifiers::SERVICE_ISSUE, [] {
    oatpp::swagger::DocumentInfo::Builder builder;

    builder
      .setTitle("Issue Traking Service")
      .setDescription("Group F Milestone 2")
      .setVersion("1.0")
      .addServer("http://localhost:8600", "server on localhost");
    return builder.build();
  }());


  /**
   *  Swagger-Ui Resources (<oatpp-examples>/lib/oatpp-swagger/res)
   */
  OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>,
    swaggerResources)(Qualifiers::SERVICE_ISSUE, [] {
    // Make sure to specify correct full path to oatpp-swagger/res folder !!!
    return oatpp::swagger::Resources::streamResources(
      "/usr/local/include/oatpp-1.3.0/bin/oatpp-swagger/res");
  }());
};

#endif /* issue_SwaggerComponent_hpp */
