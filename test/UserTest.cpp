#include <gtest/gtest.h>

#include "User.hpp"

TEST(UserTest, ConstructorAndGettersWork) {
  std::string expectedName = "Mario";
  std::string expectedRole = "Plumber/Hero";

  User user(expectedName, expectedRole);

  ASSERT_EQ(user.getName(), expectedName);
  ASSERT_EQ(user.getRole(), expectedRole);
}

TEST(UserTest, SetNameUpdatesName) {
  User user("Link", "Adventurer");
  std::string newName = "Zelda";

  user.setName(newName);

  ASSERT_EQ(user.getName(), newName);
  ASSERT_EQ(user.getRole(), "Adventurer");
}

TEST(UserTest, SetRoleUpdatesRole) {
  User user("Samus", "Hunter");
  std::string newRole = "Bounty Hunter";

  user.setRole(newRole);

  ASSERT_EQ(user.getRole(), newRole);
  ASSERT_EQ(user.getName(), "Samus");
}

TEST(UserTest, SetEmptyValues) {
  User user("Kirby", "Star Warrior");
  std::string emptyString = "";

  user.setName(emptyString);
  user.setRole(emptyString);

  ASSERT_EQ(user.getName(), emptyString);
  ASSERT_EQ(user.getRole(), emptyString);
}

TEST(UserTest, VillainToHeroTransition) {
  User user("Bowser", "Villain");
  std::string newName = "Yoshi";
  std::string newRole = "Ally";

  user.setName(newName);
  user.setRole(newRole);

  ASSERT_EQ(user.getName(), newName);
  ASSERT_EQ(user.getRole(), newRole);
}
