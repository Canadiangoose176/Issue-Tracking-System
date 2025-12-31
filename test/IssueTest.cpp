#include "Comment.hpp"
#include "Issue.hpp"
#include "gtest/gtest.h"

using std::string;

// -------------------------------
// Basic construction & ID contract
// -------------------------------

TEST(IssueModel, NewIssue_IdZero_HasNoPersistentId) {
  Issue is{0, "u1", "Crash on save", 0};
  EXPECT_FALSE(is.hasPersistentId());
  EXPECT_EQ(is.getId(), 0);
  EXPECT_EQ(is.getAuthorId(), "u1");
  EXPECT_EQ(is.getTitle(), "Crash on save");
}

TEST(IssueModel, SetIdForPersistence_OnceOnly_PositiveOnly) {
  // invalid_argument on fresh issue with non-positive id
  Issue fresh{0, "u1", "T", 0};
  EXPECT_THROW(fresh.setIdForPersistence(0), std::invalid_argument);

  // set once normally
  Issue is{0, "u1", "T", 0};
  is.setIdForPersistence(101);
  EXPECT_TRUE(is.hasPersistentId());
  EXPECT_EQ(is.getId(), 101);

  // logic_error on any subsequent set (even if value is bad)
  EXPECT_THROW(is.setIdForPersistence(202), std::logic_error);
  EXPECT_THROW(is.setIdForPersistence(0), std::logic_error);

  // ctor guard (wrap temporary to avoid macro arg parsing)
  EXPECT_THROW((Issue(-1, "u", "t", 0)), std::invalid_argument);
}

// -------------------------------
// Title rules
// -------------------------------

TEST(IssueModel, SetTitle_RejectsEmpty) {
  Issue is{0, "u1", "T", 0};
  EXPECT_NO_THROW(is.setTitle("New title"));
  EXPECT_EQ(is.getTitle(), "New title");
  EXPECT_THROW(is.setTitle(""), std::invalid_argument);
}

TEST(IssueModel, SetAuthorId_RejectsEmpty) {
  Issue is{0, "u1", "T", 0};
  is.setAuthorId("u2");
  EXPECT_EQ(is.getAuthorId(), "u2");
  EXPECT_THROW(is.setAuthorId(""), std::invalid_argument);
}

// -------------------------------
// ID-only comment list behavior
// -------------------------------

TEST(IssueModel, AddCommentId_DeDup_And_Validation) {
  Issue is{0, "u1", "T", 0};
  EXPECT_THROW(is.addComment(-5), std::invalid_argument);

  is.addComment(0);  // description allowed id
  ASSERT_EQ(is.getCommentIds().size(), 1u);
  EXPECT_EQ(is.getCommentIds()[0], 0);

  is.addComment(10);
  is.addComment(10);
  ASSERT_EQ(is.getCommentIds().size(), 2u);
  EXPECT_EQ(is.getCommentIds()[1], 10);
}

TEST(IssueModel, SetDescription_AddsIdIfMissing) {
  Issue is{0, "u1", "T", 0};
  EXPECT_FALSE(is.hasDescriptionComment());
  is.setDescriptionCommentId(42);
  EXPECT_TRUE(is.hasDescriptionComment());
  EXPECT_EQ(is.getDescriptionCommentId(), 42);
  ASSERT_EQ(is.getCommentIds().size(), 1u);
  EXPECT_EQ(is.getCommentIds()[0], 42);
}

TEST(IssueModel, DescriptionIdZeroIsValid) {
  Issue is{0, "u1", "T", 0};
  is.setDescriptionCommentId(0);
  EXPECT_TRUE(is.hasDescriptionComment());
  EXPECT_EQ(is.getDescriptionCommentId(), 0);
}

TEST(IssueModel, RemoveComment_ClearsDescriptionIfThatId) {
  Issue is{0, "u1", "T", 0};
  is.addComment(7);
  is.setDescriptionCommentId(7);
  EXPECT_TRUE(is.hasDescriptionComment());

  EXPECT_TRUE(is.removeComment(7));
  EXPECT_FALSE(is.hasDescriptionComment());
  EXPECT_TRUE(is.getCommentIds().empty());

  EXPECT_FALSE(is.removeComment(7));
}

// -------------------------------------
// Full Comment object API (new methods)
// -------------------------------------

TEST(IssueModel, AddCommentObject_StoresAndSyncsIds) {
  Issue is{0, "u1", "T", 0};

  Comment c{-1, "u2", "first text", 0};
  c.setIdForPersistence(101);

  is.addComment(c);
  ASSERT_EQ(is.getComments().size(), 1u);
  EXPECT_EQ(is.getComments()[0].getId(), 101);
  EXPECT_EQ(is.getComments()[0].getAuthor(), "u2");
  EXPECT_EQ(is.getComments()[0].getText(), "first text");

  ASSERT_EQ(is.getCommentIds().size(), 1u);
  EXPECT_EQ(is.getCommentIds()[0], 101);

  Comment upd{101, "u2", "updated text", 0};
  is.addComment(upd);
  ASSERT_EQ(is.getComments().size(), 1u);
  EXPECT_EQ(is.getComments()[0].getText(), "updated text");
}

TEST(IssueModel, FindCommentById_ConstAndMutable) {
  Issue is{0, "u1", "T", 0};
  Comment c1{-1, "a", "x", 0};
  c1.setIdForPersistence(5);
  Comment c2{-1, "b", "y", 0};
  c2.setIdForPersistence(6);
  is.addComment(c1);
  is.addComment(c2);

  const Issue& cis = is;
  const Comment* pc = cis.findCommentById(6);
  ASSERT_NE(pc, nullptr);
  EXPECT_EQ(pc->getAuthor(), "b");

  Comment* pm = is.findCommentById(5);
  ASSERT_NE(pm, nullptr);
  pm->setText("mutated");
  const Comment* pc2 = cis.findCommentById(5);
  ASSERT_NE(pc2, nullptr);
  EXPECT_EQ(pc2->getText(), "mutated");

  EXPECT_EQ(cis.findCommentById(999), nullptr);
}

// TEST(IssueModel, RemoveCommentById_
// RemovesBothAndClearsDesc) {
//   Issue is{0, "u1", "T", 0};
//   Comment c{0, "u2", "desc", 0};
//   is.addComment(c);
//   is.setDescriptionCommentId(0);

//   ASSERT_TRUE(is.hasDescriptionComment());
//   ASSERT_EQ(is.getComments().size(), 1u);
//   ASSERT_EQ(is.getCommentIds().size(), 1u);

//   EXPECT_TRUE(is.removeCommentById(0));
//   EXPECT_FALSE(is.hasDescriptionComment());
//   EXPECT_TRUE(is.getComments().empty());
//   EXPECT_TRUE(is.getCommentIds().empty());

//   EXPECT_FALSE(is.removeCommentById(0));
// }

// -------------------------------------------------
// New: extra error and rvalue paths (extra coverage)
// -------------------------------------------------

TEST(IssueModel, SetDescription_InvalidId_Throws) {
  Issue is{0, "u1", "T", 0};
  EXPECT_THROW(is.setDescriptionCommentId(-1), std::invalid_argument);
}

TEST(IssueModel, AddCommentObject_RequiresPersistedId_Throws) {
  Issue is{0, "u1", "T", 0};
  Comment draft{-1, "u2", "text", 0};
  EXPECT_THROW(is.addComment(draft), std::invalid_argument);
  EXPECT_THROW(is.addComment(Comment{-1, "u2", "text", 0}),
               std::invalid_argument);
}

TEST(IssueModel, AddCommentObject_RvaluePath_Covered) {
  Issue is{0, "u1", "T", 0};
  is.addComment(Comment{101, "u2", "rvalue text", 0});
  ASSERT_EQ(is.getComments().size(), 1u);
  EXPECT_EQ(is.getComments()[0].getId(), 101);
  EXPECT_EQ(is.getCommentIds().size(), 1u);
}

TEST(IssueModel, RemoveCommentByIdRemovesObjectsAndIds) {
  Issue is{0, "u1", "T", 0};
  Comment desc{1, "u2", "desc", 0};
  is.addComment(desc);
  is.setDescriptionCommentId(1);

  ASSERT_TRUE(is.hasDescriptionComment());
  ASSERT_EQ(is.getComments().size(), 1u);
  ASSERT_EQ(is.getCommentIds().size(), 1u);

  EXPECT_TRUE(is.removeCommentById(1));
  EXPECT_FALSE(is.hasDescriptionComment());
  EXPECT_TRUE(is.getComments().empty());
  EXPECT_TRUE(is.getCommentIds().empty());
  EXPECT_FALSE(is.removeCommentById(1));
}

TEST(IssueModel, TagLifecycleAndValidation) {
  Issue is{0, "u1", "T", 0};

  EXPECT_TRUE(is.addTag(Tag("backend", "#123456")));
  EXPECT_FALSE(is.addTag(Tag("backend", "#123456")));
  EXPECT_TRUE(is.addTag(Tag("backend", "#abcdef")));  // color update
  EXPECT_TRUE(is.hasTag("backend"));

  const auto tags = is.getTags();
  ASSERT_EQ(tags.size(), 1u);
  EXPECT_EQ(tags.front().getName(), "backend");
  EXPECT_EQ(tags.front().getColor(), "#abcdef");

  EXPECT_TRUE(is.removeTag("backend"));
  EXPECT_FALSE(is.hasTag("backend"));
  EXPECT_FALSE(is.removeTag("backend"));
  EXPECT_THROW(is.addTag(Tag("", "")), std::invalid_argument);
}

TEST(IssueModel, TimestampRejectsNegativeValues) {
  Issue is{0, "u1", "T", 0};
  EXPECT_THROW(is.setTimestamp(-5), std::invalid_argument);
  is.setTimestamp(123);
  EXPECT_EQ(is.getTimestamp(), 123);
}
