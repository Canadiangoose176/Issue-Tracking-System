#include "User.hpp"

std::string User::getName() const {
    return name;
}

std::string User::getRole() const {
    return role;
}

void User::setName(const std::string &newname) {
    name = newname;
}

void User::setRole(const std::string &newrole) {
    role = newrole;
}
