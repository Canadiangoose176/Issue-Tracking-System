// src/controller/IssueTrackerController.cpp
#include "IssueTrackerController.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <optional>
#include <stdexcept>

#include "UserRoles.hpp"

IssueTrackerController::IssueTrackerController(IssueRepository* repository)
    : repo(repository) {}

bool isValidRole(const std::string& role) {
  return user_roles::isValidRole(role);
}

Issue IssueTrackerController::createIssue(const std::string& title,
                                          const std::string& desc,
                                          const std::string& author_id) {
  if (title.empty() || author_id.empty()) {
    return Issue();
  }

  // ensure the author exists before creating any records
  try {
    repo->getUser(author_id);
  } catch (const std::exception&) {
    return Issue();
  }

  // 1. Create the Issue (no description yet, timestamp 0 means "unspecified"
  //    or "let the repo / model decide").
  Issue newIssue(0, author_id, title, 0);
  Issue savedIssue = repo->saveIssue(newIssue);

  // 2. Create the first Comment = Description
  if (!desc.empty()) {
    Comment descComment(0, author_id, desc, 0);
    Comment savedComment = repo->saveComment(savedIssue.getId(), descComment);

    // 3. Link comment #1 as description
    savedIssue.setDescriptionCommentId(savedComment.getId());
    // Keep the in-memory Issue object consistent with what was persisted.
    savedIssue.addComment(savedComment);
    repo->saveIssue(savedIssue);
  }

  return savedIssue;
}

// returns the issue from IR
Issue IssueTrackerController::getIssue(const int issueId) {
  return repo->getIssue(issueId);
}

// Updates a certain issue field and persists the updated value via repository
bool IssueTrackerController::updateIssueField(int id,
                                              const std::string& field,
                                              const std::string& value) {
  try {
    Issue issue = repo->getIssue(id);

    if (field == "title") {
      issue.setTitle(value);
      repo->saveIssue(issue);
      return true;

    } else if (field == "description") {
      // Get the description comment
      const Comment* descComment =
          issue.findCommentById(issue.getDescriptionCommentId());

      if (descComment) {
        // Update existing description comment
        Comment updated = *descComment;
        updated.setText(value);
        repo->saveComment(issue.getId(), updated);
      } else {
        // Create a new description comment
        Comment newDesc(0, issue.getAuthorId(), value, 0);
        Comment saved = repo->saveComment(issue.getId(), newDesc);
        issue.setDescriptionCommentId(saved.getId());
        repo->saveIssue(issue);
      }
      return true;

    } else if (field == "status") {
      // Normalize possible numeric input ("1"/"2"/"3") to status text,
      // in case the view or API ever passes the numeric choice instead of
      // the label.
      std::string normalized = value;

      if (normalized == "1") {
        normalized = "To Be Done";
      } else if (normalized == "2") {
        normalized = "In Progress";
      } else if (normalized == "3") {
        normalized = "Done";
      }

      // Store the status on the Issue model
      issue.setStatus(normalized);
      repo->saveIssue(issue);
      return true;

    } else if (field == "authorId" || field == "author") {
      // Ensure the new author exists before updating.
      repo->getUser(value);
      issue.setAuthorId(value);
      repo->saveIssue(issue);
      return true;

    } else {
      // Unknown field
      return false;
    }
  } catch (const std::out_of_range&) {
    return false;
  } catch (const std::invalid_argument&) {
    return false;
  }
}

// Assigns a created user to a specific issue
bool IssueTrackerController::assignUserToIssue(int issueId,
                                               const std::string& user_name) {
  try {
    // ensure user exists
    repo->getUser(user_name);

    Issue issue = repo->getIssue(issueId);
    issue.assignTo(user_name);  // must exist in Issue.hpp
    repo->saveIssue(issue);
    return true;
  } catch (const std::out_of_range&) {
    return false;
  } catch (const std::invalid_argument&) {
    return false;
  }
}

// Unassigns a user from an issue
bool IssueTrackerController::unassignUserFromIssue(int issueId) {
  try {
    Issue issue = repo->getIssue(issueId);
    issue.unassign();
    repo->saveIssue(issue);
    return true;
  } catch (const std::out_of_range&) {
    return false;
  }
}

// Deletes a certain issue and returns true if successfully deleted
bool IssueTrackerController::deleteIssue(int issueId) {
  return repo->deleteIssue(issueId);
}

// returns all comments from an issue id
std::vector<Comment> IssueTrackerController::getallComments(int issueId) {
  return repo->getAllComments(issueId);
}

// allows the view / API to display a single comment
Comment IssueTrackerController::getComment(int issueId, int commentId) {
  return repo->getComment(issueId, commentId);
}

//adds comments to certain issues and makes user inut the au-
//-thor of comments
Comment IssueTrackerController::addCommentToIssue(
    int issueId,
    const std::string& text,
    const std::string& authorId) {

  try {
    if (text.empty() || authorId.empty()) {
      return Comment();
    }

    // Ensure the issue exists
    Issue issue = repo->getIssue(issueId);

    // Ensure the user exists
    repo->getUser(authorId);

    // Create and save the new comment
    Comment newComment(-1, authorId, text, 0);  // -1 so repo assigns id
    Comment savedComment = repo->saveComment(issueId, newComment);

    // Link comment to issue and save
    issue.addComment(savedComment.getId());
    repo->saveIssue(issue);

    return savedComment;
  } catch (const std::out_of_range&) {
    return Comment();
  }
}

// allows the view / API to update a comment
bool IssueTrackerController::updateComment(int issueId,
                                           int commentId,
                                           const std::string& newText) {
  try {
    Comment comment = repo->getComment(issueId, commentId);
    comment.setText(newText);
    repo->saveComment(issueId, comment);
    return true;
  } catch (const std::out_of_range&) {
    return false;
  }
}

// deletes comments by issue id and comment id
bool IssueTrackerController::deleteComment(int issueId, int commentId) {
  try {
    repo->getComment(issueId, commentId);

    bool deleted = repo->deleteComment(issueId, commentId);

    if (deleted) {
      Issue issue = repo->getIssue(issueId);
      issue.removeComment(commentId);
      repo->saveIssue(issue);
      return true;
    }
    return false;
  } catch (const std::out_of_range&) {
    return false;
  }
}

// takes input (from view / API) and creates a new user
User IssueTrackerController::createUser(const std::string& name,
                                        const std::string& role) {
  if (name.empty() || !isValidRole(role)) {
    return User("", "");
  }
  User newUser(name, role);
  return repo->saveUser(newUser);
}

bool IssueTrackerController::updateUser(const std::string& userId,
                                        const std::string& field,
                                        const std::string& value) {
  try {
    User userObj = repo->getUser(userId);
    if (field == "name") {
      if (value.empty()) {
        return false;
      }
      if (value == userId) {
        return true;
      }

      try {
        if (repo->getUser(value).getName() == value) {
          return false;
        }
      } catch (...) {
      }

      for (Issue issue : repo->listIssues()) {
        bool issueChanged = false;
        if (issue.getAuthorId() == userId) {
          issue.setAuthorId(value);
          issueChanged = true;
        }
        if (issue.hasAssignee() && issue.getAssignedTo() == userId) {
          issue.assignTo(value);
          issueChanged = true;
        }

        for (Comment c : repo->getAllComments(issue.getId())) {
          if (c.getAuthor() == userId) {
            c.setAuthor(value);
            repo->saveComment(issue.getId(), c);
          }
        }

        if (issueChanged) {
          repo->saveIssue(issue);
        }
      }

      userObj.setName(value);
      repo->saveUser(userObj);
      repo->deleteUser(userId);
      return true;

    } else if (field == "role") {
      if (!isValidRole(value)) {
        return false;
      }
      userObj.setRole(value);
      repo->saveUser(userObj);
      return true;
    }
    return false;
  } catch (...) {
    return false;
  }
}

// removes a user from the repository
bool IssueTrackerController::removeUser(const std::string& user_name) {
  return repo->deleteUser(user_name);
}

// returns a list of all issues
std::vector<Issue> IssueTrackerController::listAllIssues() {
  return repo->listIssues();
}

// returns a list of issues that don't have an assignee
std::vector<Issue> IssueTrackerController::listAllUnassignedIssues() {
  return repo->listAllUnassigned();
}

// returns issues by username inputted
std::vector<Issue> IssueTrackerController::findIssuesByUserId(
    const std::string& user_name) {

    std::string target = user_name;
    std::transform(target.begin(), target.end(),
    target.begin(), ::tolower);

    return repo->findIssues([&](const Issue& issue) {
        std::string author = issue.getAuthorId();
        std::transform(author.begin(), author.end(),
        author.begin(), ::tolower);
        return author == target;
    });
}

std::vector<Issue> IssueTrackerController::findIssuesByStatus(
    const std::string& status) {
  std::vector<Issue> all = repo->listIssues();
  std::vector<Issue> filtered;

  for (const auto& issue : all) {
    if (issue.getStatus() == status) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

std::vector<Issue> IssueTrackerController::findIssuesByTag(
    const std::string& tag) {

  std::vector<Issue> all = repo->listIssues();
  std::vector<Issue> filtered;

  for (const auto& issue : all) {
    if (issue.hasTag(tag)) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

std::vector<Issue> IssueTrackerController::findIssuesByTags(
    const std::vector<std::string>& tags) {

  std::vector<Issue> all = repo->listIssues();
  std::vector<Issue> filtered;

  for (const auto& issue : all) {
    for (const auto& tag : tags) {
      if (issue.hasTag(tag)) {
        filtered.push_back(issue);
        break;
      }
    }
  }
  return filtered;
}

std::vector<Tag> IssueTrackerController::listAllTags() {
  return repo->listAllTags();
}

bool IssueTrackerController::deleteTagDefinition(int tagId) {
  if (tagId <= 0) {
    return false;
  }
  return repo->deleteTag(tagId);
}

bool IssueTrackerController::updateTagDefinition(int tagId, const Tag& tag) {
  if (tagId <= 0) {
    return false;
  }
  try {
    return repo->updateTagDefinition(tagId, tag);
  } catch (...) {
    return false;
  }
}

//lists all the users created
std::vector<User> IssueTrackerController::listAllUsers() {
  return repo->listAllUsers();
}

bool IssueTrackerController::addTagToIssue(int issueId,
                                           const Tag& tag) {
  try {
    return repo->addTagToIssue(issueId, tag);
  } catch (...) {
    return false;
  }
}

bool IssueTrackerController::removeTagFromIssue(int issueId,
                                                const std::string& tag) {
  try {
    return repo->removeTagFromIssue(issueId, tag);
  } catch (...) {
    return false;
  }
}

Milestone IssueTrackerController::createMilestone(
    const std::string& name,
    const std::string& desc,
    const std::string& start_date,
    const std::string& end_date) {

    if (name.empty()) {
        throw std::invalid_argument("Milestone name is required");
    }
    if (start_date.empty()) {
        throw std::invalid_argument("Milestone start date is required");
    }
    if (end_date.empty()) {
        throw std::invalid_argument("Milestone end date is required");
    }

    Milestone milestone(-1, name, desc, start_date, end_date);
    return repo->saveMilestone(milestone);
}

bool IssueTrackerController::updateMilestoneField(int milestoneId,
    const std::string& field,
    const std::string& value) {
    try {
        if (field.empty()) {
            return false;
        }

        std::optional<std::string> name;
        std::optional<std::string> desc;
        std::optional<std::string> start;
        std::optional<std::string> end;

        if (field == "name") {
            name = value;
        } else if (field == "description") {
            desc = value;
        } else if (field == "startDate" || field == "start_date") {
            start = value;
        } else if (field == "endDate" || field == "end_date") {
            end = value;
        } else {
            return false;
        }

        updateMilestone(milestoneId, name, desc, start, end);
        return true;
    } catch (...) {
        return false;
    }
}

Milestone IssueTrackerController::updateMilestone(int milestoneId,
    const std::optional<std::string>& name,
    const std::optional<std::string>& description,
    const std::optional<std::string>& startDate,
    const std::optional<std::string>& endDate) {
    if (!name && !description && !startDate && !endDate) {
        throw std::invalid_argument("No milestone fields provided");
    }

    Milestone milestone = repo->getMilestone(milestoneId);

    if (name) {
        milestone.setName(*name);
    }
    if (description) {
        milestone.setDescription(*description);
    }
    if (startDate) {
        milestone.setStartDate(*startDate);
    }
    if (endDate) {
        milestone.setEndDate(*endDate);
    }

    return repo->saveMilestone(milestone);
}

bool IssueTrackerController::addIssueToMilestone(
    int milestoneId, int issueId) {
    repo->getMilestone(milestoneId);
    repo->getIssue(issueId);
    return repo->addIssueToMilestone(milestoneId, issueId);
}


bool IssueTrackerController::removeIssueFromMilestone(
    int milestoneId, int issueId) {
    repo->getMilestone(milestoneId);
    repo->getIssue(issueId);
    return repo->removeIssueFromMilestone(milestoneId, issueId);
}

Milestone IssueTrackerController::getMilestone(int milestoneId) {
    return repo->getMilestone(milestoneId);
}


bool IssueTrackerController::deleteMilestone(
    int milestoneId, bool cascade) {
    return repo->deleteMilestone(milestoneId, cascade);
}


std::vector<Milestone> IssueTrackerController::listAllMilestones() {
    return repo->listAllMilestones();
}



std::vector<Issue> IssueTrackerController::getIssuesForMilestone(
    int milestoneId) {
    return repo->getIssuesForMilestone(milestoneId);
}
