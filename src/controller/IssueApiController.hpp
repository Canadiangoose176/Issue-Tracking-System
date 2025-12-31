#ifndef ISSUE_API_CONTROLLER_HPP_
#define ISSUE_API_CONTROLLER_HPP_

#include <algorithm>
#include <cctype>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include "Comment.hpp"
#include "CommentDto.hpp"
#include "DatabaseDto.hpp"
#include "ErrorDto.hpp"
#include "Issue.hpp"
#include "IssueDto.hpp"
#include "Milestone.hpp"
#include "MilestoneDto.hpp"
#include "TagDto.hpp"
#include "User.hpp"
#include "UserDto.hpp"
#include "UserRoles.hpp"

#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include "oatpp/web/server/api/ApiController.hpp"

#include "service/DatabaseService.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class IssueApiController : public oatpp::web::server::api::ApiController {
 private:
  std::shared_ptr<DatabaseService> dbService;

  static std::string asStdString(const oatpp::String& value) {
    return value ? *value : std::string();
  }

  static std::optional<std::string> asOptionalStdString(
      const oatpp::String& value) {
    if (!value) {
      return std::nullopt;
    }
    return std::optional<std::string>(*value);
  }

  IssueService& issues() const { return dbService->getIssueService(); }

  static std::string withDbExtension(const std::string& name) {
    if (name.size() >= 3 && name.substr(name.size() - 3) == ".db") {
      return name;
    }
    return name + ".db";
  }

  static std::string toLower(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    return out;
  }

  static std::string normalizeStatusKey(const std::string& raw) {
    std::string lowered = toLower(raw);
    std::string key;
    key.reserve(lowered.size());

    for (char ch : lowered) {
      unsigned char c = static_cast<unsigned char>(ch);
      if (!std::isspace(c) && ch != '-' && ch != '_') {
        key.push_back(static_cast<char>(c));
      }
    }
    return key;
  }

  static std::string canonicalStatusLabel(const std::string& raw) {
    const std::string key = normalizeStatusKey(raw);

    if (key == "1" || key == "tobedone") {
      return "To Be Done";
    }
    if (key == "2" || key == "inprogress") {
      return "In Progress";
    }
    if (key == "3" || key == "done") {
      return "Done";
    }

    return raw;
  }

  static bool containsWhitespace(const std::string& value) {
    return std::any_of(value.begin(), value.end(), [](unsigned char ch) {
      return std::isspace(ch);
    });
  }

  static bool isValidDate(const std::string& value) {
    // Expect basic YYYY-MM-DD; simple range checks to prevent obvious errors.
    if (value.size() != 10 || value[4] != '-' || value[7] != '-') {
      return false;
    }
    auto toInt = [](const std::string& s) -> int {
      int out = 0;
      for (char c : s) {
        if (c < '0' || c > '9') {
          return -1;
        }
        out = out * 10 + (c - '0');
      }
      return out;
    };
    int year = toInt(value.substr(0, 4));
    int month = toInt(value.substr(5, 2));
    int day = toInt(value.substr(8, 2));
    if (year <= 0 || month < 1 || month > 12 || day < 1 || day > 31) {
      return false;
    }
    // Coarse month/day check (no leap year handling needed here).
    const int maxDay =
        (month == 2) ? 29
                     : (month == 4 || month == 6 || month == 9 ||
                        month == 11)
                           ? 30
                           : 31;
    return day <= maxDay;
  }

  std::shared_ptr<OutgoingResponse> error(const Status& status,
                                          const std::string& code,
                                          const std::string& message) {
    auto dto = ErrorDto::createShared();
    dto->statusCode = status.code;
    dto->error = code.c_str();
    dto->message = message.c_str();
    return createDtoResponse(status, dto);
  }

 public:
  explicit IssueApiController(
      const std::shared_ptr<oatpp::data::mapping::ObjectMapper>&
          objectMapper)
      : oatpp::web::server::api::ApiController(objectMapper),
        dbService(std::make_shared<DatabaseService>()) {}

  static oatpp::Object<IssueDto> issueToDto(const Issue& i) {
    auto dto = IssueDto::createShared();
    dto->id = i.getId();
    dto->authorId = i.getAuthorId().c_str();
    dto->title = i.getTitle().c_str();

    dto->description = i.hasDescriptionComment()
                           ? i.getDescriptionComment().c_str()
                           : "";

    dto->assignedTo =
        i.hasAssignee() ? i.getAssignedTo().c_str() : "";

    dto->status = i.getStatus().c_str();

    auto ids = oatpp::List<oatpp::Int32>::createShared();
    for (int cid : i.getCommentIds()) {
      ids->push_back(cid);
    }
    dto->commentIds = ids;

    auto tags = oatpp::List<oatpp::Object<TagDto>>::createShared();
    for (const auto& t : i.getTags()) {
      auto tagDto = TagDto::createShared();
      if (t.getId() >= 0) {
        tagDto->id = t.getId();
      }
      tagDto->tag = t.getName().c_str();
      tagDto->color = t.getColor().c_str();
      tags->push_back(tagDto);
    }
    dto->tags = tags;

    dto->createdAt = i.getCreatedAt();
    return dto;
  }

  static oatpp::Object<CommentDto> commentToDto(const Comment& c) {
    auto dto = CommentDto::createShared();
    dto->id = c.getId();
    dto->authorId = c.getAuthor().c_str();
    dto->text = c.getText().c_str();
    dto->timestamp = c.getTimeStamp();
    return dto;
  }

  static oatpp::Object<UserDto> userToDto(const User& u) {
    auto dto = UserDto::createShared();
    dto->name = u.getName().c_str();
    dto->role = u.getRole().c_str();
    return dto;
  }

  static oatpp::Object<MilestoneDto> milestoneToDto(const Milestone& m) {
    auto dto = MilestoneDto::createShared();
    dto->id = m.getId();
    dto->name = m.getName().c_str();
    dto->description = m.getDescription().c_str();
    dto->startDate = m.getStartDate().c_str();
    dto->endDate = m.getEndDate().c_str();

    auto issueIds = oatpp::List<oatpp::Int32>::createShared();
    for (int id : m.getIssueIds()) {
      issueIds->push_back(id);
    }
    dto->issueIds = issueIds;
    return dto;
  }

  // ---- Issue endpoints ----
  ENDPOINT_INFO(createIssue) {
    info->summary = "Create a new issue";
    info->addConsumes<Object<IssueCreateDto>>("application/json");
    info->addResponse<Object<IssueDto>>(Status::CODE_201,
                                        "application/json");
    info->addResponse<Object<ErrorDto>>(
        Status::CODE_400,
        "application/json",
        "Missing required fields: title, authorId");
  }

  ENDPOINT("POST", "/issues", createIssue,
           BODY_DTO(oatpp::Object<IssueCreateDto>, body)) {
    if (!body || !body->title || !body->authorId) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "title and authorId are required");
    }

    Issue i = issues().createIssue(
        asStdString(body->title),
        asStdString(body->description),
        asStdString(body->authorId));
    if (!i.hasPersistentId()) {
      return error(Status::CODE_400,
                   "INVALID_AUTHOR",
                   "Author does not exist");
    }
    return createDtoResponse(Status::CODE_201, issueToDto(i));
  }

  ENDPOINT_INFO(listIssues) {
    info->summary = "List all issues";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/issues", listIssues) {
    auto issueList = issues().listAllIssues();
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();
    for (auto& i : issueList) {
      list->push_back(issueToDto(i));
    }
    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(listUnassignedIssues) {
    info->summary = "List all unassigned issues";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/issues/unassigned", listUnassignedIssues) {
    auto issueList = issues().listAllUnassignedIssues();
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();

    for (auto& i : issueList) {
      list->push_back(issueToDto(i));
    }
    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(getIssue) {
    info->summary = "Get an issue by id";
    info->addResponse<Object<IssueDto>>(Status::CODE_200,
                                        "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not found");
  }

  ENDPOINT("GET", "/issues/{id}", getIssue,
           PATH(oatpp::Int32, id)) {
    try {
      Issue i = issues().getIssue(id);
      return createDtoResponse(Status::CODE_200, issueToDto(i));
    } catch (...) {
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Issue not found");
    }
  }

  ENDPOINT_INFO(updateIssue) {
    info->summary = "Update a specific issue field";
    info->addConsumes<Object<IssueUpdateFieldDto>>("application/json");
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Issue updated");
    info->addResponse<Object<ErrorDto>>(
        Status::CODE_400,
        "application/json",
        "Unable to update issue");
  }

  ENDPOINT("PATCH", "/issues/{id}", updateIssue,
           PATH(oatpp::Int32, id),
           BODY_DTO(oatpp::Object<IssueUpdateFieldDto>, body)) {
    bool ok = issues().updateIssueField(
        id, asStdString(body->field), asStdString(body->value));
    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_400,
                      "UPDATE_FAILED",
                      "Unable to update issue");
  }

  ENDPOINT_INFO(deleteIssue) {
    info->summary = "Delete an issue by id";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Issue deleted (no content)");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not found");
  }

  ENDPOINT("DELETE", "/issues/{id}", deleteIssue,
           PATH(oatpp::Int32, id)) {
    bool ok = issues().deleteIssue(id);
    if (ok) {
      return createResponse(Status::CODE_204, "");
    }

    return error(Status::CODE_404,
                 "ISSUE_NOT_FOUND",
                 "Issue not found");
  }

  // ---- Comment endpoints ----

  ENDPOINT_INFO(addComment) {
    info->summary = "Add a comment to an issue";
    info->addConsumes<Object<CommentCreateDto>>("application/json");
    info->addResponse<Object<CommentDto>>(Status::CODE_201,
                                          "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing fields");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue or author not found");
  }

  ENDPOINT("POST", "/issues/{id}/comments", addComment,
           PATH(oatpp::Int32, id),
           BODY_DTO(oatpp::Object<CommentCreateDto>, body)) {
    if (!body || !body->text || !body->authorId) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "text and authorId are required");
    }

    Comment c = issues().addCommentToIssue(
        id,
        asStdString(body->text),
        asStdString(body->authorId));

    if (!c.hasPersistentId()) {
      return error(Status::CODE_404,
                   "COMMENT_NOT_CREATED",
                   "Issue or author not found");
    }
    return createDtoResponse(Status::CODE_201, commentToDto(c));
  }

  ENDPOINT_INFO(listComments) {
    info->summary = "List comments for an issue";
    info->addResponse<List<Object<CommentDto>>>(
        Status::CODE_200, "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not found");
  }

  ENDPOINT("GET", "/issues/{id}/comments", listComments,
           PATH(oatpp::Int32, id)) {
    try {
      auto comments = issues().getAllComments(id);
      auto list =
          oatpp::List<oatpp::Object<CommentDto>>::createShared();

      for (auto& c : comments) {
        // Skip the synthetic description comment (id == 0).
        if (c.getId() <= 0) {
          continue;
        }
        list->push_back(commentToDto(c));
      }
      return createDtoResponse(Status::CODE_200, list);
    } catch (...) {
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Issue not found");
    }
  }

  ENDPOINT_INFO(updateComment) {
    info->summary = "Update a comment";
    info->addConsumes<Object<CommentUpdateDto>>("application/json");
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Comment updated");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing required fields");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Comment not found");
  }

  ENDPOINT("PATCH", "/issues/{issueId}/comments/{commentId}",
           updateComment,
           PATH(oatpp::Int32, issueId),
           PATH(oatpp::Int32, commentId),
           BODY_DTO(oatpp::Object<CommentUpdateDto>, body)) {
    if (!body || !body->text) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "text is required");
    }

    bool ok = issues().updateComment(
        issueId, commentId, asStdString(body->text));

    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_404,
                      "COMMENT_NOT_FOUND",
                      "Comment not found");
  }

  ENDPOINT_INFO(deleteComment) {
    info->summary = "Delete a comment";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Comment deleted");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Comment not found");
  }

  ENDPOINT("DELETE", "/issues/{issueId}/comments/{commentId}",
           deleteComment,
           PATH(oatpp::Int32, issueId),
           PATH(oatpp::Int32, commentId)) {
    bool ok = issues().deleteComment(issueId, commentId);
    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_404,
                      "COMMENT_NOT_FOUND",
                      "Comment not found");
  }

  // ---- User endpoints ----

  ENDPOINT_INFO(createUser) {
    info->summary = "Create a new user";
    info->addConsumes<Object<UserCreateDto>>("application/json");
    info->addResponse<Object<UserDto>>(Status::CODE_201,
                                       "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Invalid name or role");
  }

  ENDPOINT("POST", "/users", createUser,
           BODY_DTO(oatpp::Object<UserCreateDto>, body)) {
    if (!body || !body->name || !body->role) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "name and role are required");
    }
    const std::string name = asStdString(body->name);
    const std::string role = asStdString(body->role);
    if (name.empty()) {
      return error(Status::CODE_400,
                   "INVALID_USER",
                   "Invalid name or role");
    }
    if (containsWhitespace(name)) {
      return error(Status::CODE_400,
                   "INVALID_USER",
                   "Username cannot contain spaces");
    }
    if (!user_roles::isValidRole(role)) {
      return error(Status::CODE_400,
                   "INVALID_USER",
                   "Invalid name or role");
    }

    User u = issues().createUser(name, role);

    return createDtoResponse(Status::CODE_201, userToDto(u));
  }

  ENDPOINT_INFO(listUsers) {
    info->summary = "List all users";
    info->addResponse<List<Object<UserDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/users", listUsers) {
    auto usersList = issues().listAllUsers();
    auto list = oatpp::List<oatpp::Object<UserDto>>::createShared();
    for (auto& u : usersList) {
      list->push_back(userToDto(u));
    }
    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(listUserRoles) {
    info->summary = "List all allowed user roles";
    info->addResponse<List<String>>(Status::CODE_200,
                                    "application/json");
  }

  ENDPOINT("GET", "/users/roles", listUserRoles) {
    auto roles = oatpp::List<oatpp::String>::createShared();
    for (const char* role : user_roles::allowedRoles()) {
      roles->push_back(role);
    }
    return createDtoResponse(Status::CODE_200, roles);
  }

  ENDPOINT_INFO(updateUser) {
    info->summary = "Update a user field";
    info->addConsumes<Object<UserUpdateDto>>("application/json");
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "User updated");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Unable to update user");
  }

  ENDPOINT("PATCH", "/users/{id}", updateUser,
           PATH(oatpp::String, id),
           BODY_DTO(oatpp::Object<UserUpdateDto>, body)) {
    if (!body || !body->field || !body->value) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "field and value are required");
    }

    const std::string field = asStdString(body->field);
    const std::string value = asStdString(body->value);

    if (field == "name" && containsWhitespace(value)) {
      return error(Status::CODE_400,
                   "INVALID_USER",
                   "Username cannot contain spaces");
    }

    bool ok = issues().updateUser(asStdString(id), field, value);
    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_400,
                      "UPDATE_FAILED",
                      "Unable to update user");
  }

  ENDPOINT_INFO(deleteUser) {
    info->summary = "Delete a user";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "User deleted");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "User not found");
  }

  ENDPOINT("DELETE", "/users/{id}", deleteUser,
           PATH(oatpp::String, id)) {
    bool ok = issues().removeUser(asStdString(id));
    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_404,
                      "USER_NOT_FOUND",
                      "User not found");
  }

  ENDPOINT_INFO(listIssuesByUser) {
    info->summary = "List issues created or assigned to a user";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "User not found");
  }

  ENDPOINT("GET", "/users/{id}/issues", listIssuesByUser,
           PATH(oatpp::String, id)) {
    std::string input = toLower(asStdString(id));
    std::string realId;
    bool found = false;

    auto allUsers = issues().listAllUsers();
    for (const auto& user : allUsers) {
      if (toLower(user.getName()) == input) {
        realId = user.getName();
        found = true;
        break;
      }
    }

    if (!found) {
      return error(Status::CODE_404,
                   "USER_NOT_FOUND",
                   "User not found");
    }

    auto userIssues = issues().findIssuesByUserId(realId);
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();
    for (auto& issue : userIssues) {
      list->push_back(issueToDto(issue));
    }

    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(assignUserToIssue) {
    info->summary = "Assign a user to an issue";
    info->addConsumes<Object<AssignIssueDto>>("application/json");
    info->addResponse<Object<IssueDto>>(Status::CODE_200,
                                        "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing issueId");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "User or issue not found");
  }

  ENDPOINT("POST", "/users/{id}/issues", assignUserToIssue,
           PATH(oatpp::String, id),
           BODY_DTO(oatpp::Object<AssignIssueDto>, body)) {
    if (!body || !body->issueId) {
      return error(Status::CODE_400,
                   "MISSING_ISSUE_ID",
                   "issueId is required");
    }

    std::string inputUser = toLower(asStdString(id));
    std::string realUser;
    bool found = false;

    for (const auto& user : issues().listAllUsers()) {
      if (toLower(user.getName()) == inputUser) {
        realUser = user.getName();
        found = true;
        break;
      }
    }

    if (!found) {
      return error(Status::CODE_404,
                   "USER_NOT_FOUND",
                   "User not found");
    }

    bool ok = issues().assignUserToIssue(body->issueId, realUser);
    if (!ok) {
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Issue not found or assignment failed");
    }

    try {
      Issue updated = issues().getIssue(body->issueId);
      return createDtoResponse(Status::CODE_200, issueToDto(updated));
    } catch (...) {
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Issue not found after assignment");
    }
  }

  ENDPOINT("PATCH", "/issues/{issueId}/unassign", unassignIssue,
           PATH(oatpp::Int32, issueId)) {
    bool ok = issues().unassignUserFromIssue(issueId);

    if (!ok) {
      return createResponse(
          Status::CODE_404,
          "Issue not found or cannot unassign");
    }

    try {
      Issue updated = issues().getIssue(issueId);
      return createDtoResponse(Status::CODE_200, issueToDto(updated));
    } catch (...) {
      return createResponse(Status::CODE_500, "Unexpected error");
    }
  }

  // ---- Tag endpoints ----

  ENDPOINT_INFO(addTag) {
    info->summary = "Add a tag to an issue";
    info->addConsumes<Object<TagDto>>("application/json");
    info->addResponse<String>(Status::CODE_201,
                              "text/plain",
                              "Tag added");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing or invalid tag");
  }

  ENDPOINT("POST", "/issues/{id}/tags", addTag,
           PATH(oatpp::Int32, id),
           BODY_DTO(oatpp::Object<TagDto>, body)) {
    if (!body || !body->tag) {
      return error(Status::CODE_400,
                   "MISSING_TAG",
                   "Missing tag");
    }

    const std::string tag = asStdString(body->tag);
    if (tag.empty()) {
      return error(Status::CODE_400,
                   "MISSING_TAG",
                   "Missing tag");
    }

    const std::string color =
        body->color ? asStdString(body->color) : std::string();

    bool ok = issues().addTagToIssue(id, Tag(tag, color));

    return ok ? createResponse(Status::CODE_201, "Tag added")
              : error(Status::CODE_400,
                      "TAG_ADD_FAILED",
                      "Failed to add tag");
  }

  ENDPOINT_INFO(removeTag) {
    info->summary = "Remove a tag from an issue";
    info->addConsumes<Object<TagDto>>("application/json");
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Tag removed");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Tag not found on issue");
  }

  ENDPOINT("DELETE", "/issues/{id}/tags", removeTag,
           PATH(oatpp::Int32, id),
           BODY_DTO(oatpp::Object<TagDto>, body)) {
    if (!body || !body->tag) {
      return error(Status::CODE_400,
                   "MISSING_TAG",
                   "Missing tag");
    }

    const std::string tag = asStdString(body->tag);
    if (tag.empty()) {
      return error(Status::CODE_400,
                   "MISSING_TAG",
                   "Missing tag");
    }

    bool ok = issues().removeTagFromIssue(id, tag);

    return ok ? createResponse(Status::CODE_204, "")
              : error(Status::CODE_404,
                      "TAG_NOT_FOUND",
                      "Tag not found on issue");
  }

  ENDPOINT_INFO(listTags) {
    info->summary = "List tags for an issue";
    info->addResponse<List<Object<TagDto>>>(
        Status::CODE_200, "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not found");
  }

  ENDPOINT("GET", "/issues/{id}/tags", listTags,
           PATH(oatpp::Int32, id)) {
    try {
      Issue issue = issues().getIssue(id);

      auto list =
          oatpp::List<oatpp::Object<TagDto>>::createShared();
      for (const auto& tag : issue.getTags()) {
        auto dto = TagDto::createShared();
        if (tag.getId() >= 0) {
          dto->id = tag.getId();
        }
        dto->tag = tag.getName().c_str();
        dto->color = tag.getColor().c_str();
        list->push_back(dto);
      }

      return createDtoResponse(Status::CODE_200, list);
    } catch (...) {
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Issue not found");
    }
  }

  ENDPOINT_INFO(listAllTags) {
    info->summary = "List all tag definitions";
    info->addResponse<List<Object<TagDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/tags", listAllTags) {
    auto list = oatpp::List<oatpp::Object<TagDto>>::createShared();
    for (const auto& tag : issues().listAllTags()) {
      auto dto = TagDto::createShared();
      if (tag.getId() >= 0) {
        dto->id = tag.getId();
      }
      dto->tag = tag.getName().c_str();
      dto->color = tag.getColor().c_str();
      list->push_back(dto);
    }
    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(deleteTagDefinition) {
    info->summary = "Delete a tag definition everywhere";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "Tag deleted");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing tag");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Tag not found");
  }

  ENDPOINT("DELETE", "/tags/{tagId}", deleteTagDefinition,
           PATH(oatpp::Int32, tagId)) {
    if (tagId <= 0) {
      return error(Status::CODE_400,
                   "MISSING_TAG",
                   "Missing tag id");
    }
    bool ok = issues().deleteTagDefinition(tagId);
    return ok ? createResponse(Status::CODE_204, "Tag deleted")
              : error(Status::CODE_404,
                      "TAG_NOT_FOUND",
                      "Tag not found");
  }

  ENDPOINT_INFO(updateTagDefinition) {
    info->summary = "Update a tag definition";
    info->addConsumes<Object<TagDto>>("application/json");
    info->addResponse<Object<TagDto>>(Status::CODE_200,
                                      "application/json",
                                      "Updated tag");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing tag id or payload");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Tag not found");
  }

  ENDPOINT("PATCH", "/tags/{tagId}", updateTagDefinition,
           PATH(oatpp::Int32, tagId),
           BODY_DTO(oatpp::Object<TagDto>, body)) {
    if (tagId <= 0 || !body) {
      return error(Status::CODE_400,
                   "INVALID_TAG",
                   "Missing tag id or payload");
    }

    std::string name = body->tag ? asStdString(body->tag) : "";
    std::string color = body->color ? asStdString(body->color) : "";

    if (name.empty() && color.empty()) {
      return error(Status::CODE_400,
                   "INVALID_TAG",
                   "Provide a new name or color");
    }

    bool ok = issues().updateTagDefinition(
        tagId, Tag(name, color));
    if (!ok) {
      return error(Status::CODE_404,
                   "TAG_NOT_FOUND",
                   "Tag not found");
    }

    auto list = issues().listAllTags();
    auto it = std::find_if(
        list.begin(), list.end(),
        [tagId](const Tag& t) { return t.getId() == tagId; });
    if (it == list.end()) {
      return error(Status::CODE_404,
                   "TAG_NOT_FOUND",
                   "Tag not found");
    }

    auto dto = TagDto::createShared();
    if (it->getId() >= 0) {
      dto->id = it->getId();
    }
    dto->tag = it->getName().c_str();
    dto->color = it->getColor().c_str();
    return createDtoResponse(Status::CODE_200, dto);
  }

  ENDPOINT_INFO(getIssuesByTag) {
    info->summary = "Find issues with a specific tag";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/issues/tags/{tag}", getIssuesByTag,
           PATH(oatpp::String, tag)) {
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();

    if (!tag) {
      return createDtoResponse(Status::CODE_200, list);
    }

    std::string searchTag = asStdString(tag);
    if (searchTag.empty()) {
      return createDtoResponse(Status::CODE_200, list);
    }

    auto allIssues = issues().listAllIssues();

    for (const auto& issue : allIssues) {
      if (issue.hasTag(searchTag)) {
        list->push_back(issueToDto(issue));
      }
    }

    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(getIssuesByTags) {
    info->summary =
        "Find issues that match any of the provided tags";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/issues/tags", getIssuesByTags,
           QUERY(oatpp::String, tags)) {
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();

    if (!tags) {
      return createDtoResponse(Status::CODE_200, list);
    }

    std::string tagsStr = asStdString(tags);
    if (tagsStr.empty()) {
      return createDtoResponse(Status::CODE_200, list);
    }

    std::vector<std::string> searchTags;
    std::istringstream iss(tagsStr);
    std::string tag;
    while (std::getline(iss, tag, ',')) {
      tag.erase(0, tag.find_first_not_of(" \t"));
      tag.erase(tag.find_last_not_of(" \t") + 1);
      if (!tag.empty()) {
        searchTags.push_back(tag);
      }
    }

    if (searchTags.empty()) {
      return createDtoResponse(Status::CODE_200, list);
    }

    auto allIssues = issues().listAllIssues();

    for (const auto& issue : allIssues) {
      for (const auto& searchTag : searchTags) {
        if (issue.hasTag(searchTag)) {
          list->push_back(issueToDto(issue));
          break;
        }
      }
    }

    return createDtoResponse(Status::CODE_200, list);
  }

  // ---- Milestone endpoints ----

  ENDPOINT_INFO(createMilestone) {
    info->summary = "Create a milestone";
    info->addConsumes<Object<MilestoneCreateDto>>("application/json");
    info->addResponse<Object<MilestoneDto>>(Status::CODE_201,
                                            "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Invalid milestone payload");
  }

  ENDPOINT("POST", "/milestones", createMilestone,
           BODY_DTO(oatpp::Object<MilestoneCreateDto>, body)) {
    if (!body) {
      return error(Status::CODE_400,
                   "MISSING_PAYLOAD",
                   "Milestone payload is required");
    }

    const std::string name = asStdString(body->name);
    const std::string start = asStdString(body->startDate);
    const std::string end = asStdString(body->endDate);

    if (name.empty() || start.empty() || end.empty()) {
      return error(Status::CODE_400,
                   "MISSING_FIELDS",
                   "name, startDate, and endDate are required");
    }
    if (!isValidDate(start) || !isValidDate(end)) {
      return error(Status::CODE_400,
                   "INVALID_DATE",
                   "startDate and endDate must be YYYY-MM-DD");
    }

    const std::string desc = asStdString(body->description);

    try {
      Milestone m =
          issues().createMilestone(name, desc, start, end);
      return createDtoResponse(Status::CODE_201, milestoneToDto(m));
    } catch (const std::invalid_argument& ex) {
      return error(Status::CODE_400,
                   "INVALID_MILESTONE",
                   ex.what());
    }
  }

  ENDPOINT_INFO(listMilestones) {
    info->summary = "List all milestones";
    info->addResponse<List<Object<MilestoneDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/milestones", listMilestones) {
    auto list = issues().listAllMilestones();

    auto dtoList =
        oatpp::List<oatpp::Object<MilestoneDto>>::createShared();
    for (auto& m : list) {
      dtoList->push_back(milestoneToDto(m));
    }

    return createDtoResponse(Status::CODE_200, dtoList);
  }

  ENDPOINT_INFO(getMilestone) {
    info->summary = "Get a milestone by id";
    info->addResponse<Object<MilestoneDto>>(Status::CODE_200,
                                            "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Milestone not found");
  }

  ENDPOINT("GET", "/milestones/{id}", getMilestone,
           PATH(oatpp::Int32, id)) {
    try {
      auto m = issues().getMilestone(id);
      return createDtoResponse(Status::CODE_200, milestoneToDto(m));
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    }
  }

  ENDPOINT_INFO(updateMilestone) {
    info->summary = "Update milestone fields";
    info->addConsumes<Object<MilestoneUpdateDto>>("application/json");
    info->addResponse<Object<MilestoneDto>>(Status::CODE_200,
                                            "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Invalid milestone data");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Milestone not found");
  }

  ENDPOINT("PATCH", "/milestones/{id}", updateMilestone,
           PATH(oatpp::Int32, id),
           BODY_DTO(oatpp::Object<MilestoneUpdateDto>, body)) {
    if (!body) {
      return error(Status::CODE_400,
                   "MISSING_PAYLOAD",
                   "Milestone payload is required");
    }

    if (!body->name && !body->description &&
        !body->startDate && !body->endDate) {
      return error(Status::CODE_400,
                   "NO_FIELDS",
                   "No fields to update");
    }

    if (body->startDate && !asStdString(body->startDate).empty() &&
        !isValidDate(asStdString(body->startDate))) {
      return error(Status::CODE_400,
                   "INVALID_DATE",
                   "startDate must be YYYY-MM-DD");
    }
    if (body->endDate && !asStdString(body->endDate).empty() &&
        !isValidDate(asStdString(body->endDate))) {
      return error(Status::CODE_400,
                   "INVALID_DATE",
                   "endDate must be YYYY-MM-DD");
    }

    try {
      auto updated = issues().updateMilestone(
          id,
          asOptionalStdString(body->name),
          asOptionalStdString(body->description),
          asOptionalStdString(body->startDate),
          asOptionalStdString(body->endDate));
      return createDtoResponse(Status::CODE_200,
                               milestoneToDto(updated));
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    } catch (const std::invalid_argument& ex) {
      (void)ex;
      return error(Status::CODE_404,
                   "ISSUE_NOT_FOUND",
                   "Milestone or issue not found");
    }
  }

  ENDPOINT_INFO(deleteMilestone) {
    info->summary = "Delete a milestone";
    info->addResponse<String>(Status::CODE_200,
                              "text/plain",
                              "Deleted");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Milestone not found");
  }

  ENDPOINT("DELETE", "/milestones/{id}", deleteMilestone,
           PATH(oatpp::Int32, id),
           QUERY(oatpp::Boolean, cascade)) {
    try {
      bool ok = issues().deleteMilestone(id, cascade);
      return createResponse(Status::CODE_200,
                            ok ? "Deleted" : "Failed");
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    }
  }

  ENDPOINT_INFO(addIssueToMilestone) {
    info->summary = "Link an issue to a milestone";
    info->addResponse<Object<MilestoneDto>>(Status::CODE_200,
                                            "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Issue already linked");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Milestone or issue not found");
  }

  ENDPOINT("POST", "/milestones/{id}/issues/{issueId}",
           addIssueToMilestone,
           PATH(oatpp::Int32, id),
           PATH(oatpp::Int32, issueId)) {
    try {
      bool linked = issues().addIssueToMilestone(id, issueId);
      if (!linked) {
        return error(Status::CODE_400,
                     "ISSUE_ALREADY_LINKED",
                     "Issue already linked");
      }
      auto milestone = issues().getMilestone(id);
      return createDtoResponse(Status::CODE_200,
                               milestoneToDto(milestone));
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    } catch (const std::invalid_argument& ex) {
      return error(Status::CODE_400,
                   "INVALID_MILESTONE",
                   ex.what());
    }
  }

  ENDPOINT_INFO(removeIssueFromMilestone) {
    info->summary = "Unlink an issue from a milestone";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not linked or "
                                        "milestone not found");
  }

  ENDPOINT("DELETE", "/milestones/{id}/issues/{issueId}",
           removeIssueFromMilestone,
           PATH(oatpp::Int32, id),
           PATH(oatpp::Int32, issueId)) {
    try {
      bool ok = issues().removeIssueFromMilestone(id, issueId);
      return ok ? createResponse(Status::CODE_204, "")
                : error(Status::CODE_404,
                        "ISSUE_NOT_LINKED",
                        "Issue not linked to milestone");
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    }
  }

  ENDPOINT_INFO(getMilestoneIssues) {
    info->summary = "List issues for a milestone";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Milestone not found");
  }

  ENDPOINT("GET", "/milestones/{id}/issues", getMilestoneIssues,
           PATH(oatpp::Int32, id)) {
    try {
      auto list = issues().getIssuesForMilestone(id);

      auto dtoList =
          oatpp::List<oatpp::Object<IssueDto>>::createShared();
      for (auto& i : list) {
        dtoList->push_back(issueToDto(i));
      }

      return createDtoResponse(Status::CODE_200, dtoList);
    } catch (const std::out_of_range&) {
      return error(Status::CODE_404,
                   "MILESTONE_NOT_FOUND",
                   "Milestone not found");
    }
  }

  // ---- Database endpoints ----

  ENDPOINT_INFO(listDatabases) {
    info->summary = "List available databases";
    info->addResponse<List<Object<DatabaseDto>>>(
        Status::CODE_200, "application/json");
  }

  ENDPOINT("GET", "/databases", listDatabases) {
    auto databases = dbService->listDatabases();
    std::string active = dbService->getActiveDatabaseName();

    auto list =
        oatpp::List<oatpp::Object<DatabaseDto>>::createShared();
    bool activeIncluded = false;

    for (const auto& name : databases) {
      auto dto = DatabaseDto::createShared();
      dto->name = name.c_str();
      dto->active = (name == active);
      activeIncluded = activeIncluded || dto->active;
      list->push_back(dto);
    }

    if (!activeIncluded && !active.empty()) {
      auto dto = DatabaseDto::createShared();
      dto->name = active.c_str();
      dto->active = true;
      list->push_back(dto);
    }

    return createDtoResponse(Status::CODE_200, list);
  }

  ENDPOINT_INFO(createDatabase) {
    info->summary = "Create a new database file";
    info->addConsumes<Object<DatabaseCreateDto>>("application/json");
    info->addResponse<Object<DatabaseDto>>(Status::CODE_201,
                                           "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Invalid database name");
    info->addResponse<Object<ErrorDto>>(Status::CODE_409,
                                        "application/json",
                                        "Database already exists");
  }

  ENDPOINT("POST", "/databases", createDatabase,
           BODY_DTO(oatpp::Object<DatabaseCreateDto>, body)) {
    if (!body || !body->name) {
      return error(Status::CODE_400,
                   "MISSING_NAME",
                   "Database name is required");
    }
    std::string provided = asStdString(body->name);
    std::string normalized = withDbExtension(provided);
    auto existing = dbService->listDatabases();
    bool alreadyExists = std::find(existing.begin(), existing.end(),
                                   normalized) != existing.end();

    bool created = dbService->createDatabase(provided);
    if (created) {
      auto dto = DatabaseDto::createShared();
      dto->name = normalized.c_str();
      dto->active =
          normalized == dbService->getActiveDatabaseName();
      return createDtoResponse(Status::CODE_201, dto);
    }

    if (alreadyExists) {
      return error(Status::CODE_409,
                   "DATABASE_EXISTS",
                   "Database already exists");
    }
    return error(Status::CODE_400,
                 "DATABASE_CREATE_FAILED",
                 "Unable to create database");
  }

  ENDPOINT_INFO(deleteDatabase) {
    info->summary = "Delete a database";
    info->addResponse<String>(Status::CODE_204,
                              "text/plain",
                              "");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Database not found");
    info->addResponse<Object<ErrorDto>>(Status::CODE_409,
                                        "application/json",
                                        "Cannot delete active database");
  }

  ENDPOINT("DELETE", "/databases/{name}", deleteDatabase,
           PATH(oatpp::String, name)) {
    std::string provided = asStdString(name);
    std::string normalized = withDbExtension(provided);

    auto existing = dbService->listDatabases();
    std::string active = dbService->getActiveDatabaseName();
    bool exists = std::find(existing.begin(), existing.end(),
                            normalized) != existing.end();

    if (!exists) {
      return error(Status::CODE_404,
                   "DATABASE_NOT_FOUND",
                   "Database not found");
    }
    if (normalized == active) {
      return error(Status::CODE_409,
                   "DATABASE_ACTIVE",
                   "Cannot delete the active database");
    }

    bool deleted = dbService->deleteDatabase(provided);
    return deleted ? createResponse(Status::CODE_204, "")
                   : error(Status::CODE_400,
                           "DATABASE_DELETE_FAILED",
                           "Unable to delete database");
  }

  ENDPOINT_INFO(renameDatabase) {
    info->summary = "Rename a database";
    info->addConsumes<Object<DatabaseRenameDto>>("application/json");
    info->addResponse<Object<DatabaseDto>>(Status::CODE_200,
                                           "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Database not found");
    info->addResponse<Object<ErrorDto>>(Status::CODE_409,
                                        "application/json",
                                        "Target name already exists");
  }

  ENDPOINT("PATCH", "/databases/{name}", renameDatabase,
           PATH(oatpp::String, name),
           BODY_DTO(oatpp::Object<DatabaseRenameDto>, body)) {
    if (!body || !body->name) {
      return error(Status::CODE_400,
                   "MISSING_NAME",
                   "New database name is required");
    }

    const std::string current = asStdString(name);
    const std::string proposed = asStdString(body->name);
    const std::string currentNormalized =
        withDbExtension(current);
    const std::string targetNormalized =
        withDbExtension(proposed);

    auto existing = dbService->listDatabases();
    const bool currentExists =
        std::find(existing.begin(), existing.end(),
                  currentNormalized) != existing.end();
    const bool targetExists =
        std::find(existing.begin(), existing.end(),
                  targetNormalized) != existing.end();

    if (!currentExists) {
      return error(Status::CODE_404,
                   "DATABASE_NOT_FOUND",
                   "Database not found");
    }
    if (targetExists && currentNormalized != targetNormalized) {
      return error(Status::CODE_409,
                   "DATABASE_EXISTS",
                   "Database already exists");
    }

    bool ok = dbService->renameDatabase(current, proposed);
    if (!ok) {
      return error(Status::CODE_400,
                   "DATABASE_RENAME_FAILED",
                   "Unable to rename database");
    }

    auto dto = DatabaseDto::createShared();
    dto->name = withDbExtension(proposed).c_str();
    dto->active =
        dto->name == dbService->getActiveDatabaseName();
    return createDtoResponse(Status::CODE_200, dto);
  }

  ENDPOINT_INFO(switchDatabase) {
    info->summary = "Switch the active database";
    info->addResponse<Object<DatabaseDto>>(Status::CODE_200,
                                           "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Database not found");
  }

  ENDPOINT("POST", "/databases/{name}/switch", switchDatabase,
           PATH(oatpp::String, name)) {
    std::string provided = asStdString(name);
    bool ok = dbService->switchDatabase(provided);
    if (!ok) {
      return error(Status::CODE_404,
                   "DATABASE_NOT_FOUND",
                   "Database not found");
    }

    auto dto = DatabaseDto::createShared();
    dto->name = dbService->getActiveDatabaseName().c_str();
    dto->active = true;
    return createDtoResponse(Status::CODE_200, dto);
  }

  // ---- Status endpoints ----

  ENDPOINT_INFO(updateIssueStatus) {
    info->summary = "Update issue status";
    info->addResponse<String>(Status::CODE_200,
                              "text/plain",
                              "Status updated");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Missing or invalid status");
    info->addResponse<Object<ErrorDto>>(Status::CODE_404,
                                        "application/json",
                                        "Issue not found");
  }

  ENDPOINT("PUT", "/issues/{id}/status", updateIssueStatus,
           PATH(Int32, id),
           BODY_STRING(String, status)) {
    if (!status) {
      return error(Status::CODE_400,
                   "MISSING_STATUS",
                   "Status is required");
    }

    const std::string canonical =
        canonicalStatusLabel(asStdString(status));

    bool ok = issues().updateIssueField(id, "status", canonical);

    return ok ? createResponse(Status::CODE_200, "Status updated")
              : error(Status::CODE_404,
                      "ISSUE_NOT_FOUND",
                      "Issue not found");
  }

  ENDPOINT_INFO(getIssuesByStatus) {
    info->summary = "List issues filtered by status";
    info->addResponse<List<Object<IssueDto>>>(
        Status::CODE_200, "application/json");
    info->addResponse<Object<ErrorDto>>(Status::CODE_400,
                                        "application/json",
                                        "Invalid status value");
  }

  ENDPOINT("GET", "/issues/status/{status}", getIssuesByStatus,
           PATH(oatpp::String, status)) {
    auto list = oatpp::List<oatpp::Object<IssueDto>>::createShared();

    if (!status) {
      return createDtoResponse(Status::CODE_200, list);
    }

    const std::string canonical =
        canonicalStatusLabel(asStdString(status));

    if (canonical != "To Be Done" &&
        canonical != "In Progress" &&
        canonical != "Done") {
      return error(
          Status::CODE_400,
          "INVALID_STATUS",
          "Status must be 'To Be Done', 'In Progress', "
          "'Done', or a valid alias (e.g., '1', '2', "
          "'tobedone').");
    }

    auto all = issues().listAllIssues();
    for (const auto& issue : all) {
      if (canonicalStatusLabel(issue.getStatus()) == canonical) {
        list->push_back(issueToDto(issue));
      }
    }

    return createDtoResponse(Status::CODE_200, list);
  }
};

#include OATPP_CODEGEN_END(ApiController)

#endif  // ISSUE_API_CONTROLLER_HPP_
