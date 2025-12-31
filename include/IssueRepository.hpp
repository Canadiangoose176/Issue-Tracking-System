#ifndef ISSUE_REPOSITORY_H_INCLUDED
#define ISSUE_REPOSITORY_H_INCLUDED

#include <functional>
#include <string>
#include <vector>

#include "Comment.hpp"
#include "Issue.hpp"
#include "User.hpp"
#include "Milestone.hpp"
#include "Tag.hpp"

/**
 * @brief Abstract repository interface for issue tracking data operations
 *
 * Defines all persistence operations for issues, comments, users,
 * tags, and milestones.
 */
class IssueRepository {
 public:
  // ===================== ISSUES =====================

  /// Get a single issue by ID
  virtual Issue getIssue(int issueId) const = 0;

  /// Create or update an issue
  virtual Issue saveIssue(const Issue& issue) = 0;

  /// Delete an issue by ID
  virtual bool deleteIssue(int issueId) = 0;

  /// List all issues
  virtual std::vector<Issue> listIssues() const = 0;

  /// Find issues by custom predicate
  virtual std::vector<Issue> findIssues(
      std::function<bool(const Issue&)> criteria) const = 0;

  /// Find issues assigned to a specific user
  virtual std::vector<Issue> findIssues(
      const std::string& userId) const;

  /// List all unassigned issues
  virtual std::vector<Issue> listAllUnassigned() const;

  // ===================== TAGS =====================

  /// Add a tag to an issue
  virtual bool addTagToIssue(int issueId,
                             const Tag& tag);

  /// Remove a tag from an issue
  virtual bool removeTagFromIssue(int issueId,
                                  const std::string& tag);

  /// Update/replace a tag on an issue
  virtual bool updateTagOnIssue(int issueId,
                                const std::string& currentTag,
                                const Tag& updatedTag) {
    (void)issueId;
    (void)currentTag;
    (void)updatedTag;
    return false;
  }

  /// List all tag definitions (name + color)
  virtual std::vector<Tag> listAllTags() const { return {}; }

  /// Delete a tag definition everywhere (removes it from all issues)
  virtual bool deleteTag(int tagId) { (void)tagId; return false; }

  /// Update a tag definition (name and/or color) by id
  virtual bool updateTagDefinition(int tagId, const Tag& tag) {
    (void)tagId;
    (void)tag;
    return false;
  }

  // ===================== COMMENTS =====================

  /// Get a specific comment by ID
  virtual Comment getComment(int issueId,
                             int commentId) const = 0;

  /// List all comments on an issue
  virtual std::vector<Comment> getAllComments(
      int issueId) const = 0;

  /// Create or update a comment
  virtual Comment saveComment(int issueId,
                              const Comment& comment) = 0;

  /// Delete a comment
  virtual bool deleteComment(int issueId,
                             int commentId) = 0;

  // ===================== USERS =====================

  /// Get a user by ID
  virtual User getUser(const std::string& userId) const = 0;

  /// Create or update a user
  virtual User saveUser(const User& user) = 0;

  /// Delete a user
  virtual bool deleteUser(const std::string& userId) = 0;

  /// List all users
  virtual std::vector<User> listAllUsers() const = 0;

  // ===================== MILESTONES =====================

  /// Create or update a milestone
  virtual Milestone saveMilestone(
      const Milestone& milestone) = 0;

  /// Get a milestone by ID
  virtual Milestone getMilestone(
      int milestoneId) const = 0;

  /// Delete a milestone
  virtual bool deleteMilestone(
      int milestoneId,
      bool cascade = false) = 0;

  /// List all milestones
  virtual std::vector<Milestone> listAllMilestones() const = 0;

  /// Add an issue to a milestone
  virtual bool addIssueToMilestone(
      int milestoneId,
      int issueId) = 0;

  /// Remove an issue from a milestone
  virtual bool removeIssueFromMilestone(
      int milestoneId,
      int issueId) = 0;

  /// Get all issues attached to a milestone
  virtual std::vector<Issue> getIssuesForMilestone(
      int milestoneId) const = 0;

  /// Virtual destructor
  virtual ~IssueRepository() = default;
};

/**
 * Factory method to create repository
 */
IssueRepository* createIssueRepository();

#endif  // ISSUE_REPOSITORY_H_INCLUDED
