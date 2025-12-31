// include/IssueTrackerController.hpp
#ifndef ISSUE_TRACKER_CONTROLLER_H
#define ISSUE_TRACKER_CONTROLLER_H

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "Comment.hpp"
#include "Issue.hpp"
#include "IssueRepository.hpp"
#include "Milestone.hpp"
#include "User.hpp"

/**
 * @brief Main controller for the issue tracking system
 *
 * The IssueTrackerController acts as the intermediary between the view
 * (or REST layer) and the data layer. It handles business logic and
 * coordinates operations between issues, comments, and users in the system.
 */
class IssueTrackerController {
 private:
  IssueRepository* repo;  ///< Repository for data persistence operations

 public:
  /**
   * @brief Constructs a new controller with the given repository
   *
   * @param repository Pointer to the issue repository for data access
   */
  IssueTrackerController(IssueRepository* repository);

  /**
   * @brief Virtual destructor for proper inheritance support
   */
  virtual ~IssueTrackerController() = default;

  /**
   * @brief Creates a new issue in the system
   *
   * @param title The title/summary of the issue
   * @param desc The detailed description of the issue
   * @param assignedTo The user ID to assign the issue to
   * @return Issue The newly created issue object
   */
  virtual Issue createIssue(const std::string& title, const std::string& desc,
                            const std::string& assignedTo);

  /**
   * @brief Retrieves an issue by its ID
   *
   * @param issueId The unique identifier of the issue
   * @return Issue The requested issue object
   */
  virtual Issue getIssue(const int issueId);

  /**
   * @brief Updates a specific field of an issue
   *
   * @param id The ID of the issue to update
   * @param field The field name to update ("title" or "description")
   * @param value The new value for the field
   * @return bool True if update was successful, false otherwise
   */
  virtual bool updateIssueField(int id, const std::string& field,
                                const std::string& value);

  /**
   * @brief Assigns a user to an existing issue
   *
   * @param issueId The ID of the issue to assign
   * @param user The user ID to assign to the issue
   * @return bool True if assignment was successful, false otherwise
   */
  virtual bool assignUserToIssue(int issueId, const std::string& user);

  /**
   * @brief Removes assignment from an issue
   *
   * @param issueId The ID of the issue to unassign
   * @return bool True if unassignment was successful, false otherwise
   */
  virtual bool unassignUserFromIssue(int issueId);

  /**
   * @brief Deletes an issue from the system
   *
   * @param id The ID of the issue to delete
   * @return bool True if deletion was successful, false otherwise
   */
  virtual bool deleteIssue(int id);

  /**
   * @brief Gets all comments for a specific issue
   *
   * @param issueId The ID of the issue
   * @return std::vector<Comment> List of all comments on the issue
   */
  virtual std::vector<Comment> getallComments(int issueId);

  /**
   * @brief Retrieves a specific comment from an issue
   *
   * @param issueId The ID of the issue containing the comment
   * @param commentId The ID of the comment to retrieve
   * @return Comment The requested comment object
   */
  virtual Comment getComment(int issueId, int commentId);

  /**
   * @brief Adds a new comment to an issue
   *
   * @param issueId The ID of the issue to comment on
   * @param text The content of the comment
   * @param authorId The ID of the user creating the comment
   * @return Comment The newly created comment object
   */
  virtual Comment addCommentToIssue(int issueId, const std::string& text,
                                    const std::string& authorId);

  /**
   * @brief Updates the text of an existing comment
   *
   * @param issueId The ID of the issue containing the comment
   * @param commentId The ID of the comment to update
   * @param newText The new text content for the comment
   * @return bool True if update was successful, false otherwise
   */
  virtual bool updateComment(int issueId, int commentId,
                             const std::string& newText);

  /**
   * @brief Deletes a comment from an issue
   *
   * @param issueId The ID of the issue containing the comment
   * @param commentId The ID of the comment to delete
   * @return bool True if deletion was successful, false otherwise
   */
  virtual bool deleteComment(int issueId, int commentId);

  /**
   * @brief Creates a new user in the system
   *
   * @param name The unique name/identifier for the user
   * @param role The role assigned to the user
   * @return User The newly created user object
   */
  virtual User createUser(const std::string& name, const std::string& role);

  /**
   * @brief Updates a user's information
   *
   * @param user The ID of the user to update
   * @param field The field to update ("name" or "role")
   * @param value The new value for the field
   * @return bool True if update was successful, false otherwise
   */
  virtual bool updateUser(const std::string& user, const std::string& field,
                          const std::string& value);

  /**
   * @brief Removes a user from the system
   *
   * @param user_name The ID of the user to remove
   * @return bool True if removal was successful, false otherwise
   */
  virtual bool removeUser(const std::string& user_name);

  /**
   * @brief Gets all issues in the system
   *
   * @return std::vector<Issue> List of all issues
   */
  virtual std::vector<Issue> listAllIssues();

  /**
   * @brief Gets all unassigned issues
   *
   * @return std::vector<Issue> List of issues with no assignee
   */
  virtual std::vector<Issue> listAllUnassignedIssues();

  /**
   * @brief Finds issues assigned to a specific user
   *
   * @param user_name The ID of the user to search for
   * @return std::vector<Issue> List of issues assigned to the user
   */
  virtual std::vector<Issue> findIssuesByUserId(const std::string& user_name);

        /**
     * @brief Find issues that have a specific tag
     * @param status The status to search for
     * @return Vector of issues with the specified tag
     */
  virtual std::vector<Issue> findIssuesByStatus(const std::string& status);

      /**
     * @brief Find issues that have a specific tag
     * @param tag The tag to search for
     * @return Vector of issues with the specified tag
     */
  std::vector<Issue> findIssuesByTag(const std::string& tag);

    /**
     * @brief Find issues that have any of the specified tags (OR logic)
     * @param tags Vector of tags to search for
     * @return Vector of issues that have at least one of the specified tags
     */
  std::vector<Issue> findIssuesByTags(const std::vector<std::string>& tags);

  /**
   * @brief List all tag definitions in the system
   */
  std::vector<Tag> listAllTags();

  /**
   * @brief Delete a tag definition (removes from all issues)
   */
  bool deleteTagDefinition(int tagId);

  /**
   * @brief Update a tag definition (name and/or color)
   */
  bool updateTagDefinition(int tagId, const Tag& tag);

  /**
   * @brief Gets all users in the system
   *
   * @return std::vector<User> List of all registered users
   */
  virtual std::vector<User> listAllUsers();

  bool addTagToIssue(int issueId, const Tag& tag);

  bool removeTagFromIssue(int issueId, const std::string& tag);
  Milestone createMilestone(const std::string& name, const std::string& desc,
                            const std::string& start_date,
                            const std::string& end_date);

  /**
   * Get a milestone by its ID
   * @param milestoneId - The milestone ID
   * @return Milestone object with all associated issues
   * @throws std::invalid_argument if milestone doesn't exist
   */
  Milestone getMilestone(int milestoneId);

  /**
   * Update a milestone's information
   * @param milestoneId - The milestone ID
   * @param field - Field to update ("name", "description", "start_date",
   * "end_date")
   * @param value - New value for the field
   * @return true if successful, false otherwise
   */
  bool updateMilestoneField(int milestoneId, const std::string& field,
                            const std::string& value);

  /**
   * @brief Partially update milestone data.
   *
   * @param milestoneId The milestone id to patch.
   * @param name Optional new name (non-empty when provided).
   * @param description Optional description text.
   * @param startDate Optional new start date (non-empty when provided).
   * @param endDate Optional new end date (non-empty when provided).
   * @return Updated milestone.
   */
  Milestone updateMilestone(int milestoneId,
                            const std::optional<std::string>& name,
                            const std::optional<std::string>& description,
                            const std::optional<std::string>& startDate,
                            const std::optional<std::string>& endDate);

  /**
   * Delete a milestone
   * @param milestoneId - The milestone ID to delete
   * @param cascade - If true, also delete all associated issues
   * @return true if successfully deleted, false otherwise
   */
  bool deleteMilestone(int milestoneId, bool cascade = false);

  /**
   * List all milestones in the system
   * @return Vector of all Milestone objects
   */
  std::vector<Milestone> listAllMilestones();

  /**
   * Add an existing issue to a milestone
   * @param milestoneId - The milestone ID
   * @param issueId - The issue ID to add
   * @return true if successful, false otherwise
   */
  bool addIssueToMilestone(int milestoneId, int issueId);

  /**
   * Remove an issue from a milestone
   * @param milestoneId - The milestone ID
   * @param issueId - The issue ID to remove
   * @return true if successful, false otherwise
   */
  bool removeIssueFromMilestone(int milestoneId, int issueId);

  /**
   * Get all issues belonging to a specific milestone
   * @param milestoneId - The milestone ID
   * @return Vector of Issue objects in this milestone
   */
  std::vector<Issue> getIssuesForMilestone(int milestoneId);

  /**
   * Get milestone statistics (issue count, completion percentage, etc.)
   * @param milestoneId - The milestone ID
   * @return Statistics as formatted string or structured data
   */
  std::string getMilestoneStatistics(int milestoneId);

  /**
   * Find milestones by date range
   * @param startDate - Start date filter (optional, empty for no filter)
   * @param endDate - End date filter (optional, empty for no filter)
   * @return Vector of Milestone objects within the date range
   */
  std::vector<Milestone> findMilestonesByDateRange(const std::string& startDate,
                                                   const std::string& endDate);

  /**
   * Get active milestones (current date is between start and end date)
   * @return Vector of currently active Milestone objects
   */
  std::vector<Milestone> getActiveMilestones();

  /**
   * Get completed milestones (all issues closed)
   * @return Vector of completed Milestone objects
   */
  std::vector<Milestone> getCompletedMilestones();
};

#endif  // ISSUE_TRACKER_CONTROLLER_H
