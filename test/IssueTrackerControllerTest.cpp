#include <cstdlib>
#include <memory>

#include "IssueTrackerController.hpp"
#include "SQLiteIssueRepository.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::SizeIs;
using ::testing::Throw;

class MockIssueRepository : public IssueRepository {
 public:
  MOCK_METHOD(Issue, saveIssue, (const Issue& issue), (override));
  MOCK_METHOD(Issue, getIssue, (int id), (const, override));
  MOCK_METHOD(bool, deleteIssue, (int id), (override));
  MOCK_METHOD(std::vector<Issue>, listIssues, (), (const, override));
  MOCK_METHOD(std::vector<Issue>, findIssues,
              (std::function<bool(const Issue&)> criteria), (const, override));
  MOCK_METHOD(std::vector<Issue>, findIssues, (const std::string& userId),
              (const, override));
  MOCK_METHOD(std::vector<Issue>, listAllUnassigned, (), (const, override));

  MOCK_METHOD(bool, addTagToIssue, (int issueId, const Tag& tag),
              (override));
  MOCK_METHOD(bool, removeTagFromIssue, (int issueId, const std::string& tag),
              (override));

  MOCK_METHOD(Comment, saveComment, (int issueId, const Comment& comment),
              (override));
  MOCK_METHOD(Comment, getComment, (int issueId, int commentId),
              (const, override));
  MOCK_METHOD(bool, deleteComment, (int issueId, int commentId), (override));
  MOCK_METHOD(std::vector<Comment>, getAllComments, (int issueId),
              (const, override));

  MOCK_METHOD(User, saveUser, (const User& user), (override));
  MOCK_METHOD(User, getUser, (const std::string& id), (const, override));
  MOCK_METHOD(bool, deleteUser, (const std::string& id), (override));
  MOCK_METHOD(std::vector<User>, listAllUsers, (), (const, override));

  MOCK_METHOD(Milestone, saveMilestone, (const Milestone& milestone),
              (override));
  MOCK_METHOD(Milestone, getMilestone, (int milestoneId), (const, override));
  MOCK_METHOD(bool, deleteMilestone, (int milestoneId, bool cascade),
              (override));
  MOCK_METHOD(std::vector<Milestone>, listAllMilestones, (), (const, override));
  MOCK_METHOD(bool, addIssueToMilestone, (int milestoneId, int issueId),
              (override));
  MOCK_METHOD(bool, removeIssueFromMilestone, (int milestoneId, int issueId),
              (override));
  MOCK_METHOD(std::vector<Issue>, getIssuesForMilestone, (int milestoneId),
              (const, override));
};

TEST(IssueTrackerControllerTest, CreateIssueValid) {
  MockIssueRepository mockRepo;
  Issue persistedIssue(1, "user123", "title", 0);
  Comment descComment(1, "user123", "desc", 0);
  User author("user123", "Developer");

  testing::InSequence seq;
  EXPECT_CALL(mockRepo, getUser("user123")).WillOnce(testing::Return(author));
  EXPECT_CALL(mockRepo, saveIssue(testing::_))
      .WillOnce(testing::Return(persistedIssue));
  EXPECT_CALL(mockRepo, saveComment(1, testing::_))
      .WillOnce(testing::Return(descComment));
  EXPECT_CALL(mockRepo, saveIssue(testing::_))
      .WillOnce(testing::Return(persistedIssue));

  IssueTrackerController controller(&mockRepo);
  Issue result = controller.createIssue("title", "desc", "user123");

  EXPECT_EQ(result.getAuthorId(), "user123");
}

TEST(IssueTrackerControllerTest, UpdateIssueFieldTitleSuccess) {
  MockIssueRepository mockRepo;
  Issue existingIssue(1, "user", "old", 0);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(existingIssue));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "title", "newTitle");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UpdateIssueFieldDescriptionSuccess) {
  MockIssueRepository mockRepo;
  Issue existingIssue(1, "user", "title", 0);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(existingIssue));
  EXPECT_CALL(mockRepo, saveComment(1, testing::_))
      .WillOnce(testing::Return(Comment(1, "user", "newDesc", 0)));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "description", "newDesc");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UpdateIssueFieldInvalidField) {
  MockIssueRepository mockRepo;
  Issue existingIssue(1, "user", "title", 0);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(existingIssue));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "invalid", "value");

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, UpdateIssueFieldThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getIssue(1))
      .WillOnce(testing::Throw(std::invalid_argument("Not found")));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "title", "newTitle");

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, AssignUserToIssueSuccess) {
  MockIssueRepository mockRepo;
  Issue existingIssue(1, "author", "title", 0);
  User user("name", "Developer");

  EXPECT_CALL(mockRepo, getUser("user123")).WillOnce(testing::Return(user));
  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(existingIssue));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.assignUserToIssue(1, "user123");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, AssignUserToIssueThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getUser("user123"))
      .WillOnce(testing::Throw(std::out_of_range("Not found")));

  IssueTrackerController controller(&mockRepo);
  EXPECT_NO_THROW({
    bool result = controller.assignUserToIssue(1, "user123");
    EXPECT_FALSE(result);
  });
}

TEST(IssueTrackerControllerTest, UnassignUserFromIssueSuccess) {
  MockIssueRepository mockRepo;
  Issue existingIssue(1, "author", "title", 0);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(existingIssue));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.unassignUserFromIssue(1);

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UnassignUserFromIssueThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getIssue(1))
      .WillOnce(testing::Throw(std::out_of_range("Not found")));

  IssueTrackerController controller(&mockRepo);
  EXPECT_NO_THROW({
    bool result = controller.unassignUserFromIssue(1);
    EXPECT_FALSE(result);
  });
}

TEST(IssueTrackerControllerTest, DeleteIssue) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, deleteIssue(1)).WillOnce(testing::Return(true));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.deleteIssue(1);

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, ListAllIssues) {
  MockIssueRepository mockRepo;
  std::vector<Issue> issues = {Issue(1, "u1", "t1", 0),
                               Issue(2, "u2", "t2", 0)};

  EXPECT_CALL(mockRepo, listIssues()).WillOnce(testing::Return(issues));

  IssueTrackerController controller(&mockRepo);
  std::vector<Issue> result = controller.listAllIssues();

  EXPECT_EQ(result.size(), 2);
}

// TEST(IssueTrackerControllerTest, FindIssuesByUserId) {
//     MockIssueRepository mockRepo;
//     std::vector<Issue> issues = { Issue(1, "u1", "t1", 0) };

//     EXPECT_CALL(mockRepo, findIssues("u1"))
//         .WillOnce(testing::Return(issues));

//     IssueTrackerController controller(&mockRepo);
//     std::vector<Issue> result = controller.findIssuesByUserId("u1");

//     EXPECT_EQ(result.size(), 1);
// }

TEST(IssueTrackerControllerTest, FindIssuesByUserIdUsesCaseInsensitiveMatch) {
  MockIssueRepository mockRepo;
  EXPECT_CALL(mockRepo,
              findIssues(testing::A<std::function<bool(const Issue&)>>()))
      .WillOnce(testing::Invoke([](std::function<bool(const Issue&)> filter) {
        Issue issue(5, "UserX", "t1", 0);
        std::vector<Issue> matches;
        if (filter(issue)) {
          matches.push_back(issue);
        }
        return matches;
      }));

  IssueTrackerController controller(&mockRepo);
  auto result = controller.findIssuesByUserId("userx");

  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result[0].getAuthorId(), "UserX");
}

TEST(IssueTrackerControllerTest, AddCommentToIssueSuccess) {
  MockIssueRepository mockRepo;
  Issue issue(1, "author", "title", 0);
  User user("author", "Developer");
  Comment comment(1, "author", "text", 1);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(issue));
  EXPECT_CALL(mockRepo, getUser("author")).WillOnce(testing::Return(user));
  EXPECT_CALL(mockRepo, saveComment(1, testing::_))
      .WillOnce(testing::Return(comment));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  Comment result = controller.addCommentToIssue(1, "text", "author");

  EXPECT_EQ(result.getAuthor(), "author");
}

TEST(IssueTrackerControllerTest, UpdateCommentSuccess) {
  MockIssueRepository mockRepo;
  Comment comment(1, "author", "oldText", 1);

  EXPECT_CALL(mockRepo, getComment(1, 1)).WillOnce(testing::Return(comment));
  EXPECT_CALL(mockRepo, saveComment(1, testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateComment(1, 1, "newText");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UpdateCommentThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getComment(1, 1))
      .WillOnce(testing::Throw(std::out_of_range("Not found")));

  IssueTrackerController controller(&mockRepo);
  EXPECT_NO_THROW({
    bool result = controller.updateComment(1, 1, "text");
    EXPECT_FALSE(result);
  });
}

TEST(IssueTrackerControllerTest, DeleteCommentSuccess) {
  MockIssueRepository mockRepo;
  Comment comment(5, "author", "text", 1);
  Issue issue(1, "author", "title", 0);

  EXPECT_CALL(mockRepo, getComment(1, 5)).WillOnce(testing::Return(comment));
  EXPECT_CALL(mockRepo, deleteComment(1, 5)).WillOnce(testing::Return(true));
  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(issue));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.deleteComment(1, 5);

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, DeleteCommentThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getComment(1, 5))
      .WillOnce(testing::Throw(std::out_of_range("Not found")));

  IssueTrackerController controller(&mockRepo);
  EXPECT_NO_THROW({
    bool result = controller.deleteComment(1, 5);
    EXPECT_FALSE(result);
  });
}

TEST(IssueTrackerControllerTest, CreateUserSuccess) {
  MockIssueRepository mockRepo;
  User user("name", "Developer");

  EXPECT_CALL(mockRepo, saveUser(testing::_)).WillOnce(testing::Return(user));

  IssueTrackerController controller(&mockRepo);
  User result = controller.createUser("name", "Developer");

  EXPECT_EQ(result.getName(), "name");
}

TEST(IssueTrackerControllerTest, CreateUserEmptyName) {
  MockIssueRepository mockRepo;
  IssueTrackerController controller(&mockRepo);
  User result = controller.createUser("", "");

  EXPECT_EQ(result.getName(), "");
}

TEST(IssueTrackerControllerTest, CreateUserInvalidRoleRejected) {
  MockIssueRepository mockRepo;
  EXPECT_CALL(mockRepo, saveUser(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  User result = controller.createUser("name", "InvalidRole");

  EXPECT_EQ(result.getName(), "");
}

TEST(IssueTrackerControllerTest, UpdateUserRoleSuccess) {
  MockIssueRepository mockRepo;
  User user("oldName", "Developer");

  EXPECT_CALL(mockRepo, getUser("user123")).WillOnce(testing::Return(user));
  EXPECT_CALL(mockRepo, saveUser(testing::_))
      .WillOnce(testing::Return(User("oldName", "Owner")));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateUser("user123", "role", "Owner");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UpdateUserRoleRejectsInvalidRole) {
  MockIssueRepository mockRepo;
  User user("oldName", "Developer");

  EXPECT_CALL(mockRepo, getUser("user123")).WillOnce(testing::Return(user));
  EXPECT_CALL(mockRepo, saveUser(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateUser("user123", "role", "Bad");

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, UpdateUserInvalidField) {
  MockIssueRepository mockRepo;
  User user("name", "Developer");

  EXPECT_CALL(mockRepo, getUser("user123")).WillOnce(testing::Return(user));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateUser("user123", "invalid", "value");

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, UpdateUserThrows) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getUser("user123"))
      .WillOnce(testing::Throw(std::invalid_argument("Not found")));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateUser("user123", "name", "newName");

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, UpdateUserNameRenamesInPlace) {
  SQLiteIssueRepository repo(":memory:");
  IssueTrackerController controller(&repo);

  repo.saveUser(User("old", "Developer"));
  Issue created = controller.createIssue("title", "desc", "old");
  EXPECT_TRUE(controller.assignUserToIssue(created.getId(), "old"));
  controller.addCommentToIssue(created.getId(), "comment", "old");

  EXPECT_TRUE(controller.updateUser("old", "name", "new"));

  EXPECT_THROW(repo.getUser("old"), std::invalid_argument);
  User renamed = repo.getUser("new");
  EXPECT_EQ(renamed.getRole(), "Developer");

  Issue reloaded = repo.getIssue(created.getId());
  EXPECT_EQ(reloaded.getAuthorId(), "new");
  ASSERT_TRUE(reloaded.hasAssignee());
  EXPECT_EQ(reloaded.getAssignedTo(), "new");

  auto comments = repo.getAllComments(created.getId());
  ASSERT_FALSE(comments.empty());
  for (const auto& c : comments) {
    EXPECT_EQ(c.getAuthor(), "new");
  }
}

TEST(IssueTrackerControllerTest, RemoveUser) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, deleteUser("123")).WillOnce(testing::Return(true));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.removeUser("123");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, ListAllUsers) {
  MockIssueRepository mockRepo;
  std::vector<User> users = {User("u1", "Developer"), User("u2", "Owner")};

  EXPECT_CALL(mockRepo, listAllUsers()).WillOnce(testing::Return(users));

  IssueTrackerController controller(&mockRepo);
  std::vector<User> result = controller.listAllUsers();

  EXPECT_EQ(result.size(), 2);
}

TEST(IssueTrackerControllerTest, ListAllUnassignedIssues) {
  MockIssueRepository mockRepo;
  std::vector<Issue> issues = {Issue(1, "u1", "t1", 0)};

  EXPECT_CALL(mockRepo, listAllUnassigned()).WillOnce(testing::Return(issues));

  IssueTrackerController controller(&mockRepo);
  std::vector<Issue> result = controller.listAllUnassignedIssues();

  EXPECT_EQ(result.size(), 1);
}

TEST(IssueTrackerControllerTest, CreateIssueInvalidInputReturnsEmptyIssue) {
  MockIssueRepository mockRepo;
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  Issue result = controller.createIssue("", "desc", "user123");

  EXPECT_EQ(result.getId(), 0);
  EXPECT_EQ(result.getTitle(), "");
}

TEST(IssueTrackerControllerTest, CreateIssueRejectsUnknownAuthor) {
  MockIssueRepository mockRepo;

  EXPECT_CALL(mockRepo, getUser("ghost"))
      .WillOnce(Throw(std::invalid_argument("missing user")));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  Issue result = controller.createIssue("title", "desc", "ghost");

  EXPECT_FALSE(result.hasPersistentId());
  EXPECT_EQ(result.getAuthorId(), "");
}

TEST(IssueTrackerControllerTest,
     UpdateIssueFieldCreatesDescriptionWhenMissing) {
  MockIssueRepository mockRepo;
  Issue issue(1, "user", "title", 0);

  testing::InSequence seq;
  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(issue));
  EXPECT_CALL(mockRepo, saveComment(1, testing::_))
      .WillOnce(testing::Return(Comment(2, "user", "newDesc", 1)));
  EXPECT_CALL(mockRepo, saveIssue(testing::_)).Times(1);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "description", "newDesc");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, UpdateIssueFieldStatusPersists) {
  MockIssueRepository mockRepo;
  Issue issue(1, "user", "title", 0);

  EXPECT_CALL(mockRepo, getIssue(1)).WillOnce(testing::Return(issue));
  EXPECT_CALL(mockRepo, saveIssue(testing::Truly([](const Issue& i) {
                return i.getStatus() == "In Progress";
              })));

  IssueTrackerController controller(&mockRepo);
  bool result = controller.updateIssueField(1, "status", "In Progress");

  EXPECT_TRUE(result);
}

TEST(IssueTrackerControllerTest, AddCommentToIssueRejectsEmptyFields) {
  MockIssueRepository mockRepo;
  EXPECT_CALL(mockRepo, getIssue(testing::_)).Times(0);
  EXPECT_CALL(mockRepo, getUser(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  Comment result = controller.addCommentToIssue(1, "", "author");

  EXPECT_FALSE(result.hasPersistentId());
  EXPECT_EQ(result.getAuthor(), "");
}

TEST(IssueTrackerControllerTest, AddCommentToIssueHandlesMissingIssue) {
  MockIssueRepository mockRepo;
  EXPECT_CALL(mockRepo, getIssue(99))
      .WillOnce(testing::Throw(std::out_of_range("no issue")));

  IssueTrackerController controller(&mockRepo);
  Comment result = controller.addCommentToIssue(99, "text", "author");

  EXPECT_FALSE(result.hasPersistentId());
}

TEST(IssueTrackerControllerTest, DeleteCommentReturnsFalseWhenRepoRefuses) {
  MockIssueRepository mockRepo;
  Comment comment(5, "author", "text", 1);

  EXPECT_CALL(mockRepo, getComment(1, 5)).WillOnce(testing::Return(comment));
  EXPECT_CALL(mockRepo, deleteComment(1, 5)).WillOnce(testing::Return(false));
  EXPECT_CALL(mockRepo, getIssue(testing::_)).Times(0);

  IssueTrackerController controller(&mockRepo);
  bool result = controller.deleteComment(1, 5);

  EXPECT_FALSE(result);
}

TEST(IssueTrackerControllerTest, TagOperationsPropagateToRepository) {
  MockIssueRepository mockRepo;

  // Corrected Expectation
  EXPECT_CALL(mockRepo, addTagToIssue(
                        4,
                        testing::Truly([](const Tag& t) {
                          return t.getName() == "bug" && t.getColor().empty();
                        }))).WillOnce(testing::Return(true));

  EXPECT_CALL(mockRepo, removeTagFromIssue(4, "bug"))
      .WillOnce(testing::Throw(std::runtime_error("db failure")));

  IssueTrackerController controller(&mockRepo);
  EXPECT_TRUE(controller.addTagToIssue(4, Tag("bug", "")));
  EXPECT_FALSE(controller.removeTagFromIssue(4, "bug"));
}

TEST(IssueTrackerControllerTest, GetWrappersDelegateToRepository) {
  MockIssueRepository mockRepo;
  Issue issue(7, "author", "t", 0);
  Comment comment(1, "author", "txt", 1);
  std::vector<Comment> comments{comment};

  EXPECT_CALL(mockRepo, getIssue(7)).WillOnce(testing::Return(issue));
  EXPECT_CALL(mockRepo, getComment(7, 1)).WillOnce(testing::Return(comment));
  EXPECT_CALL(mockRepo, getAllComments(7)).WillOnce(testing::Return(comments));

  IssueTrackerController controller(&mockRepo);
  EXPECT_EQ(controller.getIssue(7).getId(), 7);
  EXPECT_EQ(controller.getComment(7, 1).getId(), 1);
  EXPECT_EQ(controller.getallComments(7).size(), 1u);
}

class IssueTrackerControllerIntegrationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    setenv("ISSUE_REPO_BACKEND", "memory", 1);
    repo = std::unique_ptr<IssueRepository>(createIssueRepository());
    controller = std::make_unique<IssueTrackerController>(repo.get());
    controller->createUser("owner", "Owner");
  }

  std::unique_ptr<IssueRepository> repo;
  std::unique_ptr<IssueTrackerController> controller;
};

TEST_F(IssueTrackerControllerIntegrationTest, FiltersByStatusAndTags) {
  Issue done = controller->createIssue("Done item", "desc", "owner");
  controller->addTagToIssue(done.getId(), Tag("backend", "#000000"));
  controller->updateIssueField(done.getId(), "status", "Done");

  controller->createIssue("Todo item", "", "owner");

  auto doneIssues = controller->findIssuesByStatus("Done");
  EXPECT_THAT(doneIssues, SizeIs(1));
  EXPECT_EQ(doneIssues.front().getId(), done.getId());

  auto tagMatches = controller->findIssuesByTag("backend");
  EXPECT_THAT(tagMatches, SizeIs(1));

  auto multiTagMatches = controller->findIssuesByTags({"backend", "api"});
  EXPECT_THAT(multiTagMatches, SizeIs(1));
}

TEST_F(IssueTrackerControllerIntegrationTest, MilestoneWorkflow) {
  Issue issue = controller->createIssue("Bug", "", "owner");
  Milestone milestone = controller->createMilestone("Sprint A", "desc",
                                                    "2024-01-01", "2024-02-01");

  EXPECT_TRUE(
      controller->addIssueToMilestone(milestone.getId(), issue.getId()));

  auto milestoneIssues = controller->getIssuesForMilestone(milestone.getId());
  EXPECT_THAT(milestoneIssues, SizeIs(1));
  EXPECT_EQ(milestoneIssues.front().getId(), issue.getId());

  EXPECT_TRUE(controller->updateMilestoneField(milestone.getId(), "description",
                                               "updated"));
  EXPECT_EQ(controller->getMilestone(milestone.getId()).getDescription(),
            "updated");

  EXPECT_TRUE(
      controller->removeIssueFromMilestone(milestone.getId(), issue.getId()));
  EXPECT_TRUE(controller->deleteMilestone(milestone.getId(), false));
  EXPECT_THROW(controller->getMilestone(milestone.getId()), std::out_of_range);
  EXPECT_NO_THROW(controller->getIssue(issue.getId()));
}

TEST_F(IssueTrackerControllerIntegrationTest,
       CascadeDeleteMilestoneRemovesIssues) {
  Issue issue = controller->createIssue("To delete", "", "owner");
  Milestone milestone = controller->createMilestone("Sprint B", "desc",
                                                    "2024-03-01", "2024-04-01");
  controller->addIssueToMilestone(milestone.getId(), issue.getId());

  EXPECT_TRUE(controller->deleteMilestone(milestone.getId(), true));
  EXPECT_THROW(controller->getIssue(issue.getId()), std::invalid_argument);
  EXPECT_THROW(controller->getMilestone(milestone.getId()), std::out_of_range);
}
