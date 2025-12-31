#ifndef TAG_DTO_HPP_
#define TAG_DTO_HPP_

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class TagDto : public oatpp::DTO {
  DTO_INIT(TagDto, DTO)

  DTO_FIELD(Int32, id);
  DTO_FIELD(oatpp::String, tag);
  DTO_FIELD(oatpp::String, color);
};

#include OATPP_CODEGEN_END(DTO)

#endif
