#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>

#include "Comment.hpp"
#include "Issue.hpp"
#include "IssueRepository.hpp"
#include "User.hpp"

using ::testing::Contains;
using ::testing::ElementsAre;
using ::testing::IsEmpty;
using ::testing::SizeIs;

class InMemoryIssueRepositoryTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setenv("ISSUE_REPO_BACKEND", "memory", 1);
    repository = std::unique_ptr<IssueRepository>(createIssueRepository());
  }

  std::unique_ptr<IssueRepository> repository;
};

TEST_F(InMemoryIssueRepositoryTest, SaveAndGetIssue) {
  Issue issue(0, "user1", "Test Issue");

  Issue saved = repository->saveIssue(issue);
  EXPECT_GT(saved.getId(), 0);
  EXPECT_EQ(saved.getTitle(), "Test Issue");

  Issue retrieved = repository->getIssue(saved.getId());
  EXPECT_EQ(retrieved.getId(), saved.getId());
  EXPECT_EQ(retrieved.getTitle(), "Test Issue");
}

TEST_F(InMemoryIssueRepositoryTest, GetNonExistentIssueThrows) {
  EXPECT_THROW(repository->getIssue(999), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, DeleteIssue) {
  Issue issue(0, "user1", "To be deleted");

  Issue saved = repository->saveIssue(issue);
  int issueId = saved.getId();

  bool deleted = repository->deleteIssue(issueId);
  EXPECT_TRUE(deleted);

  EXPECT_THROW(repository->getIssue(issueId), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, DeleteNonExistentIssueReturnsFalse) {
  bool result = repository->deleteIssue(999);
  EXPECT_FALSE(result);
}

TEST_F(InMemoryIssueRepositoryTest, ListIssues) {
  Issue issue1(0, "user1", "Issue 1");
  repository->saveIssue(issue1);

  Issue issue2(0, "user2", "Issue 2");
  repository->saveIssue(issue2);

  std::vector<Issue> issues = repository->listIssues();
  EXPECT_THAT(issues, SizeIs(2));
}

TEST_F(InMemoryIssueRepositoryTest, FindIssuesByCriteria) {
  Issue issue1(0, "user1", "Bug");
  issue1.assignTo("user3");
  repository->saveIssue(issue1);

  Issue issue2(0, "user2", "Feature");
  issue2.assignTo("user4");
  repository->saveIssue(issue2);

  auto results = repository->findIssues(
      [](const Issue& issue) { return issue.getTitle() == "Bug"; });

  EXPECT_THAT(results, SizeIs(1));
  EXPECT_EQ(results[0].getTitle(), "Bug");
}

TEST_F(InMemoryIssueRepositoryTest, FindIssuesByUserId) {
  Issue issue1(0, "user1", "Bug");
  issue1.assignTo("alice");
  repository->saveIssue(issue1);

  Issue issue2(0, "user2", "Feature");
  issue2.assignTo("bob");
  repository->saveIssue(issue2);

  Issue issue3(0, "user3", "Another");
  issue3.assignTo("alice");
  repository->saveIssue(issue3);

  auto aliceIssues = repository->findIssues("alice");
  EXPECT_THAT(aliceIssues, SizeIs(2));

  auto bobIssues = repository->findIssues("bob");
  EXPECT_THAT(bobIssues, SizeIs(1));
}

TEST_F(InMemoryIssueRepositoryTest, ListAllUnassigned) {
  Issue issue1(0, "user1", "Unassigned 1");
  repository->saveIssue(issue1);

  Issue issue2(0, "user2", "Assigned");
  issue2.assignTo("user3");
  repository->saveIssue(issue2);

  Issue issue3(0, "user4", "Unassigned 2");
  repository->saveIssue(issue3);

  auto unassigned = repository->listAllUnassigned();
  EXPECT_THAT(unassigned, SizeIs(2));

  for (const auto& issue : unassigned) {
    EXPECT_FALSE(issue.hasAssignee());
  }
}

TEST_F(InMemoryIssueRepositoryTest, SaveAndGetComment) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment(-1, "user2", "Test comment");

  Comment savedComment = repository->saveComment(savedIssue.getId(), comment);
  EXPECT_GT(savedComment.getId(), -1);

  Comment retrieved =
      repository->getComment(savedIssue.getId(), savedComment.getId());
  EXPECT_EQ(retrieved.getId(), savedComment.getId());
  EXPECT_EQ(retrieved.getText(), "Test comment");
}

TEST_F(InMemoryIssueRepositoryTest, GetCommentWithWrongIssueThrows) {
  Issue issue1(0, "user1", "Issue 1");
  Issue saved1 = repository->saveIssue(issue1);

  Issue issue2(0, "user2", "Issue 2");
  Issue saved2 = repository->saveIssue(issue2);

  Comment comment(-1, "user1", "Test comment");
  Comment saved = repository->saveComment(saved1.getId(), comment);

  EXPECT_THROW(repository->getComment(saved2.getId(), saved.getId()),
               std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, GetAllComments) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment1(-1, "user1", "First comment");
  repository->saveComment(savedIssue.getId(), comment1);

  Comment comment2(-1, "user2", "Second comment");
  repository->saveComment(savedIssue.getId(), comment2);

  auto comments = repository->getAllComments(savedIssue.getId());
  EXPECT_THAT(comments, SizeIs(2));
}

TEST_F(InMemoryIssueRepositoryTest, DeleteCommentByIssueAndId) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment(-1, "user1", "Test comment");
  Comment saved = repository->saveComment(savedIssue.getId(), comment);

  bool deleted = repository->deleteComment(savedIssue.getId(), saved.getId());
  EXPECT_TRUE(deleted);

  auto comments = repository->getAllComments(savedIssue.getId());
  EXPECT_THAT(comments, IsEmpty());
}

TEST_F(InMemoryIssueRepositoryTest, SaveAndGetUser) {
  User user("testuser", "developer");

  User saved = repository->saveUser(user);
  EXPECT_EQ(saved.getName(), "testuser");

  User retrieved = repository->getUser("testuser");
  EXPECT_EQ(retrieved.getName(), "testuser");
  EXPECT_EQ(retrieved.getRole(), "developer");
}

TEST_F(InMemoryIssueRepositoryTest, GetNonExistentUserThrows) {
  EXPECT_THROW(repository->getUser("nonexistent"), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, SaveUserWithEmptyNameThrows) {
  User user("", "role");

  EXPECT_THROW(repository->saveUser(user), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, UpdateUser) {
  User user("user1", "oldrole");
  repository->saveUser(user);

  user.setRole("newrole");
  User updated = repository->saveUser(user);

  User retrieved = repository->getUser("user1");
  EXPECT_EQ(retrieved.getRole(), "newrole");
}

TEST_F(InMemoryIssueRepositoryTest, DeleteUser) {
  User user("todelete", "role");
  repository->saveUser(user);

  bool deleted = repository->deleteUser("todelete");
  EXPECT_TRUE(deleted);

  EXPECT_THROW(repository->getUser("todelete"), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, DeleteNonExistentUserReturnsFalse) {
  bool result = repository->deleteUser("nonexistent");
  EXPECT_FALSE(result);
}

TEST_F(InMemoryIssueRepositoryTest, ListAllUsers) {
  User user1("user1", "role1");
  repository->saveUser(user1);

  User user2("user2", "role2");
  repository->saveUser(user2);

  auto users = repository->listAllUsers();
  EXPECT_THAT(users, SizeIs(2));
}

TEST_F(InMemoryIssueRepositoryTest, IssueWithCommentsHydration) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment1(-1, "user1", "Comment 1");
  repository->saveComment(savedIssue.getId(), comment1);

  Comment comment2(-1, "user2", "Comment 2");
  repository->saveComment(savedIssue.getId(), comment2);

  Issue retrieved = repository->getIssue(savedIssue.getId());
  auto comments = retrieved.getComments();
  EXPECT_THAT(comments, SizeIs(2));
}

TEST_F(InMemoryIssueRepositoryTest, UpdateIssueWithExistingId) {
  Issue issue(0, "user1", "Original");
  Issue saved = repository->saveIssue(issue);

  saved.setTitle("Updated");
  Issue updated = repository->saveIssue(saved);

  EXPECT_EQ(updated.getId(), saved.getId());
  EXPECT_EQ(updated.getTitle(), "Updated");

  Issue retrieved = repository->getIssue(saved.getId());
  EXPECT_EQ(retrieved.getTitle(), "Updated");
}

TEST_F(InMemoryIssueRepositoryTest, UpdateNonExistentIssueThrows) {
  Issue issue(999, "user1", "Non-existent");

  EXPECT_THROW(repository->saveIssue(issue), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, UpdateCommentWithExistingId) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment(0, "user1", "Original");
  Comment saved = repository->saveComment(savedIssue.getId(), comment);

  saved.setText("Updated");
  Comment updated = repository->saveComment(savedIssue.getId(), saved);

  EXPECT_EQ(updated.getId(), saved.getId());
  EXPECT_EQ(updated.getText(), "Updated");
}

TEST_F(InMemoryIssueRepositoryTest, DeleteIssueAlsoDeletesComments) {
  Issue issue(0, "user1", "Test Issue");
  Issue savedIssue = repository->saveIssue(issue);

  Comment comment(0, "user1", "Test comment");
  Comment savedComment = repository->saveComment(savedIssue.getId(), comment);

  repository->deleteIssue(savedIssue.getId());

  EXPECT_THROW(repository->getComment(savedIssue.getId(), savedComment.getId()),
               std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, CommentValidation) {
  EXPECT_THROW(Comment(-2, "user1", "text"), std::invalid_argument);
  EXPECT_THROW(Comment(-1, "", "text"), std::invalid_argument);
  EXPECT_THROW(Comment(-1, "user1", ""), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, IssueValidation) {
  EXPECT_THROW(Issue(-1, "user1", "title"), std::invalid_argument);
  EXPECT_THROW(Issue(0, "", "title"), std::invalid_argument);
  EXPECT_THROW(Issue(0, "user1", ""), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, UserRoleManagement) {
  User user("testuser", "admin");
  repository->saveUser(user);

  User retrieved = repository->getUser("testuser");
  EXPECT_EQ(retrieved.getRole(), "admin");

  user.setRole("developer");
  repository->saveUser(user);

  retrieved = repository->getUser("testuser");
  EXPECT_EQ(retrieved.getRole(), "developer");
}

TEST_F(InMemoryIssueRepositoryTest, SaveCommentWithExplicitZeroId) {
  Issue issue(0, "user1", "Test Issue");
  Issue saved = repository->saveIssue(issue);

  Comment desc(0, "user1", "Description");
  Comment stored = repository->saveComment(saved.getId(), desc);
  EXPECT_EQ(stored.getId(), 0);

  Comment fetched = repository->getComment(saved.getId(), 0);
  EXPECT_EQ(fetched.getText(), "Description");
}

TEST_F(InMemoryIssueRepositoryTest, DeleteCommentThrowsForMissingData) {
  Issue issue(0, "user1", "Test Issue");
  Issue saved = repository->saveIssue(issue);

  EXPECT_THROW(repository->deleteComment(saved.getId(), 999),
               std::invalid_argument);
  EXPECT_THROW(repository->deleteComment(999, 0), std::invalid_argument);
}

TEST_F(InMemoryIssueRepositoryTest, MilestoneLifecycleWithIssueLinks) {
  Milestone milestone(-1, "Sprint 1", "Initial", "2024-01-01", "2024-02-01");
  Milestone savedMilestone = repository->saveMilestone(milestone);
  EXPECT_GT(savedMilestone.getId(), 0);

  Issue issue(0, "user1", "Link me up");
  Issue savedIssue = repository->saveIssue(issue);

  EXPECT_TRUE(repository->addIssueToMilestone(savedMilestone.getId(),
                                              savedIssue.getId()));

  auto withIssue = repository->getMilestone(savedMilestone.getId());
  EXPECT_TRUE(withIssue.hasIssue(savedIssue.getId()));

  auto issues = repository->getIssuesForMilestone(savedMilestone.getId());
  EXPECT_THAT(issues, SizeIs(1));
  EXPECT_EQ(issues.front().getId(), savedIssue.getId());

  withIssue.setDescription("Updated");
  Milestone updated = repository->saveMilestone(withIssue);
  EXPECT_EQ(updated.getDescription(), "Updated");

  EXPECT_TRUE(repository->removeIssueFromMilestone(savedMilestone.getId(),
                                                   savedIssue.getId()));
  auto withoutIssue = repository->getMilestone(savedMilestone.getId());
  EXPECT_FALSE(withoutIssue.hasIssue(savedIssue.getId()));

  auto milestones = repository->listAllMilestones();
  EXPECT_THAT(milestones, SizeIs(1));
}

TEST_F(InMemoryIssueRepositoryTest, DeleteMilestoneCascadeRemovesIssues) {
  Milestone milestone(-1, "Sprint 2", "Cleanup", "2024-03-01", "2024-04-01");
  Milestone savedMilestone = repository->saveMilestone(milestone);

  Issue issue(0, "user1", "Temporary");
  Issue savedIssue = repository->saveIssue(issue);
  repository->addIssueToMilestone(savedMilestone.getId(), savedIssue.getId());

  EXPECT_TRUE(repository->deleteMilestone(savedMilestone.getId(), true));
  EXPECT_THROW(repository->getIssue(savedIssue.getId()), std::invalid_argument);
  EXPECT_THROW(repository->getMilestone(savedMilestone.getId()),
               std::out_of_range);
}

TEST_F(InMemoryIssueRepositoryTest, GetIssuesForMissingMilestoneThrows) {
  EXPECT_THROW(repository->getIssuesForMilestone(999), std::out_of_range);
}
