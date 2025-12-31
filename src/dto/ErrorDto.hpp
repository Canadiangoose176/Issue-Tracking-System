#ifndef ERROR_DTO_HPP_
#define ERROR_DTO_HPP_

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class ErrorDto : public oatpp::DTO {
  DTO_INIT(ErrorDto, DTO)

  DTO_FIELD(Int32, statusCode);
  DTO_FIELD(String, error);
  DTO_FIELD(String, message);
};

#include OATPP_CODEGEN_END(DTO)

#endif  // ERROR_DTO_HPP_
