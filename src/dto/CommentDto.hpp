#ifndef COMMENT_DTO_HPP_
#define COMMENT_DTO_HPP_

#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/core/Types.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class CommentDto : public oatpp::DTO {
  DTO_INIT(CommentDto, DTO)

  DTO_FIELD(oatpp::Int32, id);
  DTO_FIELD(oatpp::String, authorId, "author_id");
  DTO_FIELD(oatpp::String, text);
  DTO_FIELD(oatpp::Int64, timestamp);
};

class CommentCreateDto : public oatpp::DTO {
  DTO_INIT(CommentCreateDto, DTO)

  DTO_FIELD(oatpp::String, text);
  DTO_FIELD(oatpp::String, authorId, "author_id");
};

class CommentUpdateDto : public oatpp::DTO {
  DTO_INIT(CommentUpdateDto, DTO)

  DTO_FIELD(oatpp::String, text);
};

#include OATPP_CODEGEN_END(DTO)

#endif
