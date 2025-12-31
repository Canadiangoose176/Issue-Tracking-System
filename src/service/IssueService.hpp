#ifndef ISSUE_SERVICE_HPP_
#define ISSUE_SERVICE_HPP_

#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <utility>
#include <iostream>

#include "IssueTrackerController.hpp"
#include "IssueRepository.hpp"
#include "Issue.hpp"
#include "Comment.hpp"
#include "User.hpp"
#include "Milestone.hpp"
#include "Tag.hpp"

class IssueService {
 private:
  std::unique_ptr<IssueRepository> repo_;
  IssueTrackerController controller_;

 public:
  IssueService()
      : IssueService(std::unique_ptr<IssueRepository>(createIssueRepository())) {}

  explicit IssueService(std::unique_ptr<IssueRepository> repo)
      : repo_(std::move(repo)),
        controller_(repo_.get()) {}

  Issue createIssue(const std::string& title,
                    const std::string& desc,
                    const std::string& authorId) {
    return controller_.createIssue(title, desc, authorId);
  }

  Issue getIssue(int id) { return controller_.getIssue(id); }

  bool updateIssueField(int id,
                        const std::string& field,
                        const std::string& value) {
    return controller_.updateIssueField(id, field, value);
  }

  bool deleteIssue(int id) { return controller_.deleteIssue(id); }

  bool assignUserToIssue(int issueId, const std::string& userId) {
    return controller_.assignUserToIssue(issueId, userId);
  }

  std::vector<Issue> findIssuesByStatus(const std::string& status) {
    return controller_.findIssuesByStatus(status);
  }

  bool unassignUserFromIssue(int issueId) {
    return controller_.unassignUserFromIssue(issueId);
  }

  std::vector<Issue> listAllIssues() {
    return controller_.listAllIssues();
  }

  std::vector<Issue> listAllUnassignedIssues() {
    return controller_.listAllUnassignedIssues();
  }

  // ✅ KEEP ONLY THIS ONE
  std::vector<Issue> findIssuesByUserId(const std::string& userId) {
    return controller_.findIssuesByUserId(userId);
  }

  Comment addCommentToIssue(int issueId,
                            const std::string& text,
                            const std::string& authorId) {
    return controller_.addCommentToIssue(issueId, text, authorId);
  }

  bool updateComment(int issueId,
                     int commentId,
                     const std::string& newText) {
    return controller_.updateComment(issueId, commentId, newText);
  }

  bool deleteComment(int issueId, int commentId) {
    return controller_.deleteComment(issueId, commentId);
  }

  std::vector<Comment> getAllComments(int issueId) {
    return controller_.getallComments(issueId);
  }

  User createUser(const std::string& name, const std::string& role) {
    return controller_.createUser(name, role);
  }

  bool updateUser(const std::string& userId,
                  const std::string& field,
                  const std::string& value) {
    return controller_.updateUser(userId, field, value);
  }

  bool removeUser(const std::string& userId) {
    return controller_.removeUser(userId);
  }

  std::vector<User> listAllUsers() {
    return controller_.listAllUsers();
  }

  bool addTagToIssue(int issueId, const Tag& tag) {
    return controller_.addTagToIssue(issueId, tag);
  }
  std::vector<Tag> listAllTags() {
    return controller_.listAllTags();
  }
  bool deleteTagDefinition(int tagId) {
    return controller_.deleteTagDefinition(tagId);
  }
  bool updateTagDefinition(int tagId, const Tag& tag) {
    return controller_.updateTagDefinition(tagId, tag);
  }
  std::vector<Issue> findIssuesByTag(const std::string& tag) {
    return controller_.findIssuesByTag(tag);
  }

  std::vector<Issue> findIssuesByTags(const std::vector<std::string>& tags) {
    return controller_.findIssuesByTags(tags);
  }

  bool removeTagFromIssue(int issueId, const std::string& tag) {
    return controller_.removeTagFromIssue(issueId, tag);
  }

  Milestone createMilestone(
    const std::string& name,
    const std::string& desc,
    const std::string& start,
    const std::string& end) {
  std::cout << "Creating milestone: " << name << std::endl;
  return controller_.createMilestone(name, desc, start, end);
}

std::vector<Milestone> listAllMilestones() {
  return controller_.listAllMilestones();
}

Milestone getMilestone(int id) {
  return controller_.getMilestone(id);
}

Milestone updateMilestone(int id,
                          const std::optional<std::string>& name,
                          const std::optional<std::string>& desc,
                          const std::optional<std::string>& start,
                          const std::optional<std::string>& end) {
  return controller_.updateMilestone(id, name, desc, start, end);
}

bool deleteMilestone(int id, bool cascade) {
  return controller_.deleteMilestone(id, cascade);
}

bool addIssueToMilestone(int mId, int issueId) {
  return controller_.addIssueToMilestone(mId, issueId);
}

bool removeIssueFromMilestone(int mId, int issueId) {
  return controller_.removeIssueFromMilestone(mId, issueId);
}

std::vector<Issue> getIssuesForMilestone(int mId) {
  return controller_.getIssuesForMilestone(mId);
}

};

#endif
