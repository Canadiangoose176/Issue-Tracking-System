#ifndef USER_DTO_HPP_
#define USER_DTO_HPP_

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class UserDto : public oatpp::DTO {
  DTO_INIT(UserDto, DTO)

  DTO_FIELD(oatpp::String, name);
  DTO_FIELD(oatpp::String, role);
};

class UserCreateDto : public oatpp::DTO {
  DTO_INIT(UserCreateDto, DTO)

  DTO_FIELD(oatpp::String, name);
  DTO_FIELD(oatpp::String, role);
};

class UserUpdateDto : public oatpp::DTO {
  DTO_INIT(UserUpdateDto, DTO)

  DTO_FIELD(oatpp::String, field);
  DTO_FIELD(oatpp::String, value);
};

class AssignIssueDto : public oatpp::DTO {
  DTO_INIT(AssignIssueDto, DTO)
  DTO_FIELD(Int32, issueId);
};

#include OATPP_CODEGEN_END(DTO)

#endif
