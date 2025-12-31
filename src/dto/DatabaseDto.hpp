#ifndef DATABASE_DTO_HPP_
#define DATABASE_DTO_HPP_

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class DatabaseDto : public oatpp::DTO {
  DTO_INIT(DatabaseDto, DTO)

  DTO_FIELD(oatpp::String, name);
  DTO_FIELD(oatpp::Boolean, active);
};

class DatabaseCreateDto : public oatpp::DTO {
  DTO_INIT(DatabaseCreateDto, DTO)

  DTO_FIELD(oatpp::String, name);
};

class DatabaseRenameDto : public oatpp::DTO {
  DTO_INIT(DatabaseRenameDto, DTO)

  DTO_FIELD(oatpp::String, name, "name");
};

#include OATPP_CODEGEN_END(DTO)

#endif
