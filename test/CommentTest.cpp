#include "gtest/gtest.h"
#include "Comment.hpp"

using std::string;

// --------------------------------------
// Core happy-path tests (existing ones)
// --------------------------------------

TEST(Comment, ConstructNewWithIdNegativeOne) {
  Comment c{-1, "u1", "hello", 0};
  EXPECT_FALSE(c.hasPersistentId());
  EXPECT_EQ(c.getId(), -1);
  EXPECT_EQ(c.getAuthor(), "u1");
  EXPECT_EQ(c.getText(), "hello");
}

TEST(Comment, RepoAssignsIdOnce) {
  Comment c{-1, "u1", "t", 0};
  c.setIdForPersistence(10);
  EXPECT_TRUE(c.hasPersistentId());
  EXPECT_EQ(c.getId(), 10);
  EXPECT_THROW(c.setIdForPersistence(11), std::logic_error);
}

TEST(Comment, TextValidation) {
  Comment c{-1, "u1", "t", 0};
  EXPECT_NO_THROW(c.setText("abc"));
  EXPECT_EQ(c.getText(), "abc");
  EXPECT_THROW(c.setText(""), std::invalid_argument);
}

TEST(Comment, TimestampSet) {
  Comment c{-1, "u1", "t", 0};
  c.setTimeStamp(1234);
  EXPECT_EQ(c.getTimeStamp(), 1234);
}

// -------------------------------------------------
// New: ctor validation branches (extra coverage)
// -------------------------------------------------

TEST(Comment, CtorRejectsInvalidInputs) {
  EXPECT_THROW((Comment(-2, "u", "t", 0)), std::invalid_argument);
  EXPECT_THROW((Comment(-1, "", "t", 0)), std::invalid_argument);
  EXPECT_THROW((Comment(-1, "u", "", 0)), std::invalid_argument);
}
