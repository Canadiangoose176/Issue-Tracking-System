#ifndef MILESTONE_DTO_HPP
#define MILESTONE_DTO_HPP

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"

#include OATPP_CODEGEN_BEGIN(DTO)

class MilestoneDto : public oatpp::DTO {
  DTO_INIT(MilestoneDto, DTO)

  DTO_FIELD(Int32, id);
  DTO_FIELD(String, name);
  DTO_FIELD(String, description);
  DTO_FIELD(String, startDate);
  DTO_FIELD(String, endDate);
  DTO_FIELD(List<Int32>, issueIds);
};

class MilestoneCreateDto : public oatpp::DTO {
  DTO_INIT(MilestoneCreateDto, DTO)

  DTO_FIELD(String, name);
  DTO_FIELD(String, description);
  DTO_FIELD(String, startDate);
  DTO_FIELD(String, endDate);
};

class MilestoneUpdateDto : public oatpp::DTO {
  DTO_INIT(MilestoneUpdateDto, DTO)

  DTO_FIELD(String, name);
  DTO_FIELD(String, description);
  DTO_FIELD(String, startDate);
  DTO_FIELD(String, endDate);
};

#include OATPP_CODEGEN_END(DTO)

#endif
