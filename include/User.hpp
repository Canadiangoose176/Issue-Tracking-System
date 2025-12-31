#ifndef USER_H
#define USER_H

#include <iostream>
#include <string>

/**
 * @brief Represents a user in the issue tracking system
 *
 * The User class stores basic information about system users including
 * their name and role. Users can be assigned to issues and can create
 * comments on issues.
 */
class User {
 private:
    std::string name;    ///< Unique identifier/name for the user
    std::string role;    ///< Role (e.g., Developer, Owner, Maintainer)

 public:
    /**
     * @brief Constructs a new User object
     *
     * @param name The unique name identifier for the user
     * @param role The role assigned to the user in the system
     */
    User(std::string name, std::string role) : name(name), role(role) {}

    /**
     * @brief Gets the user's name
     *
     * @return std::string The name of the user
     */
    std::string getName() const;

    /**
     * @brief Gets the user's role
     *
     * @return std::string The role of the user
     */
    std::string getRole() const;

    /**
     * @brief Sets the user's name
     *
     * @param newname The new name to assign to the user
     */
    void setName(const std::string &newname);

    /**
     * @brief Sets the user's role
     *
     * @param newrole The new role to assign to the user
     */
    void setRole(const std::string &newrole);
};

#endif
