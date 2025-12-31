#ifndef USER_ROLES_HPP_
#define USER_ROLES_HPP_

#include <algorithm>
#include <array>
#include <string>

namespace user_roles {

// Central list of allowed roles.
inline const std::array<const char*, 3>& allowedRoles() {
  static const std::array<const char*, 3> roles{
      "Developer",
      "Owner",
      "Reporter"};
  return roles;
}

inline bool isValidRole(const std::string& role) {
  const auto& roles = allowedRoles();
  return std::any_of(roles.begin(), roles.end(),
                     [&](const char* allowed) {
                       return role == allowed;
                     });
}

}  // namespace user_roles

#endif  // USER_ROLES_HPP_
