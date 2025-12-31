#ifndef SQLITEISSUEREPOSITORY_HPP
#define SQLITEISSUEREPOSITORY_HPP

#include <sqlite3.h>

#include <functional>
#include <string>
#include <vector>

#include "IssueRepository.hpp"
#include "Milestone.hpp"


// Concrete IssueRepository implementation backed by SQLite.
class SQLiteIssueRepository : public IssueRepository {
 private:
  sqlite3* db_;

  void execOrThrow(const std::string& sql) const;
  void initializeSchema();

  bool exists(const std::string& sql,
              const std::function<void(sqlite3_stmt*)>& binder = nullptr) const;

  void forEachRow(const std::string& sql,
                  const std::function<void(sqlite3_stmt*)>& binder,
                  const std::function<void(sqlite3_stmt*)>& onRow) const;

  Comment insertCommentRow(int issueId, const Comment& comment, int commentId);
  std::vector<Comment> loadComments(int issueId) const;
  bool issueExists(int issueId) const;
  bool commentExists(int issueId, int commentId) const;
  int nextCommentIdForIssue(int issueId) const;
  std::vector<int> loadMilestoneIssueIds(int milestoneId) const;
  bool milestoneExists(int milestoneId) const;

 public:
  explicit SQLiteIssueRepository(const std::string& dbPath);
  ~SQLiteIssueRepository() override;

  // ---- Issue operations ----
  Issue getIssue(int issueId) const override;
  Issue saveIssue(const Issue& issue) override;
  bool deleteIssue(int issueId) override;

  std::vector<Issue> listIssues() const override;

  // Generic predicate-based search (used by some tests/tools).
  std::vector<Issue> findIssues(
      std::function<bool(const Issue&)> criteria) const override;

  // Milestone helpers:
  //  - all issues with no assignee
  std::vector<Issue> listAllUnassigned() const override;
  //  - all issues assigned to a particular user
  std::vector<Issue> findIssues(
      const std::string& userId) const override;

  // ---- Tag operations ----
  std::vector<Tag> listAllTags() const override;
  bool deleteTag(int tagId) override;
  bool updateTagDefinition(int tagId, const Tag& tag) override;
  bool addTagToIssue(int issueId, const Tag& tag) override;
  bool removeTagFromIssue(int issueId, const std::string& tag) override;
  bool updateTagOnIssue(int issueId,
                        const std::string& currentTag,
                        const Tag& updatedTag) override;

  // ---- Comment operations ----
  Comment getComment(int issueId, int commentId) const override;
  std::vector<Comment> getAllComments(int issueId) const override;
  Comment saveComment(int issueId, const Comment& comment) override;
  bool deleteComment(int issueId, int commentId) override;

  // ---- User operations ----
  User getUser(const std::string& userId) const override;
  User saveUser(const User& user) override;
  bool deleteUser(const std::string& userId) override;
  std::vector<User> listAllUsers() const override;

  // ==================== NEW MILESTONE METHODS ====================

  /**
   * Save a milestone to the database (create or update)
   * @param milestone - The milestone to save
   * @return The saved milestone with generated ID if new
   */
  Milestone saveMilestone(const Milestone& milestone) override;

  /**
   * Get a milestone by ID
   * @param milestoneId - The milestone ID
   * @return The milestone with all associated issues
   */
  Milestone getMilestone(int milestoneId) const override;

  /**
   * Delete a milestone
   * @param milestoneId - The milestone ID to delete
   * @param cascade - If true, also delete associated issues
   * @return true if deleted successfully
   */
  bool deleteMilestone(int milestoneId, bool cascade = false) override;

  /**
   * List all milestones
   * @return Vector of all milestones
   */
  std::vector<Milestone> listAllMilestones() const override;

  /**
   * Add an issue to a milestone
   * @param milestoneId - The milestone ID
   * @param issueId - The issue ID to add
   * @return true if successful
   */
  bool addIssueToMilestone(int milestoneId, int issueId) override;

  /**
   * Remove an issue from a milestone
   * @param milestoneId - The milestone ID
   * @param issueId - The issue ID to remove
   * @return true if successful
   */
  bool removeIssueFromMilestone(int milestoneId, int issueId) override;

  /**
   * Get all issues for a specific milestone
   * @param milestoneId - The milestone ID
   * @return Vector of issues belonging to this milestone
   */
  std::vector<Issue> getIssuesForMilestone(int milestoneId) const override;
};

#endif  // TEXT_BASED_ITS_SQLITEISSUEREPOSITORY_HPP_
