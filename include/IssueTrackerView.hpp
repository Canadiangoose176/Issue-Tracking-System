#ifndef ISSUE_TRACKER_VIEW_H
#define ISSUE_TRACKER_VIEW_H

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <cctype>
#include <ctime>

#include "IssueTrackerController.hpp"

/**
 * @brief User interface class for the issue tracking system
 *
 * The IssueTrackerView handles all user interactions and displays
 * information to the user. It provides a menu-driven interface
 * for managing issues, comments, users, tags, and statuses.
 */
class IssueTrackerView {
 private:
    IssueTrackerController* controller;  ///< Controller for business logic

    /**
     * @brief Ensures issue-dependent actions have at least one issue
     *
     * If there are no issues, the user is prompted to create one.
     *
     * @param actionName Description of the action that requires an issue
     * @return true if issues exist (or were created) and the action can proceed
     * @return false if the user decided to return to the main menu
     */
    bool ensureIssuesAvailable(const std::string& actionName);

 public:
    // === Menu Operation Methods ===

    /**
     * @brief Displays the main menu options to the user
     */
    void displayMenu();

    /**
     * @brief Handles creation of a new issue with user input
     */
    void createIssue();

    /**
     * @brief Handles updating existing issue fields (title/description/status)
     */
    void updateIssue();

    /**
     * @brief Assigns a user to an existing issue
     */
    void assignUser();

    /**
     * @brief Removes user assignment from an issue
     */
    void unassignUser();

    /**
     * @brief Deletes an issue from the system
     */
    void deleteIssue();

    /**
     * @brief Lists all issues in the system (including their status)
     */
    void listIssues();

    /**
     * @brief Lists all unassigned issues
     */
    void listUnassignedIssues();

    /**
     * @brief Finds and displays issues assigned to a specific user
     */
    void findIssuesByUser();

    /**
     * @brief Adds a comment to an issue
     */
    void addComment();

    /**
     * @brief Updates an existing comment's text
     */
    void updateComment();

    /**
     * @brief Deletes a comment from an issue
     */
    void deleteComment();

    /**
     * @brief Creates a new user in the system
     */
    void createUser();

    /**
     * @brief Lists all users in the system
     */
    void listUsers();

    /**
     * @brief Removes a user from the system
     */
    void removeUser();

    /**
     * @brief Updates user information (name or role)
     */
    void updateUser();

    /**
     * @brief Adds a description comment to an issue
     */
    void addComIssue();

    // === Helper Methods ===

    /**
     * @brief Gets a user ID through interactive selection
     *
     * @return std::string The selected user's ID (empty if none)
     */
    std::string getuserId();

    /**
     * @brief Gets an issue ID through interactive selection
     *
     * @return int The selected issue's ID, or -1 on failure
     */
    int getissueId();

    /**
     * @brief Validates and gets an integer input within [1, bound]
     *
     * @param bound The upper bound for valid input
     * @return int The validated user input
     */
    int getvalidInt(int bound);

    /**
     * @brief Displays detailed information about a specific issue
     *
     * @param id The ID of the issue to display
     * @return list of actual comment IDs shown (in printed order)
     */
    std::vector<int> displayIssue(int id);

    /**
     * @brief Constructs a new view with the given controller
     *
     * @param controller Pointer to the controller for business logic
     */
    explicit IssueTrackerView(IssueTrackerController* controller);

    /**
     * @brief Main application loop that handles user interaction
     */
    void run();

    /**
     * @brief Adds a tag to an issue
     */
    void addTag();

    /**
     * @brief Removes a tag from an issue
     */
    void removeTag();

    /**
     * @brief Shows issues grouped by status and prints counts per status
     *
     * Example output:
     *  - To Be Done: 3 issues
     *  - In Progress: 2 issues
     *  - Done: 5 issues
     */
    void viewIssuesByStatus();
};

#endif  // ISSUE_TRACKER_VIEW_H
