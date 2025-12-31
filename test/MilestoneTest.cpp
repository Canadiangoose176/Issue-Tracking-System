#include <gtest/gtest.h>

#include "Milestone.hpp"

TEST(MilestoneTests, ConstructWithValidData) {
  Milestone milestone(5, "Sprint 1", "Stabilize MVP", "2024-01-01",
                      "2024-02-01", {1, 2});
  EXPECT_EQ(milestone.getId(), 5);
  EXPECT_EQ(milestone.getName(), "Sprint 1");
  EXPECT_EQ(milestone.getDescription(), "Stabilize MVP");
  EXPECT_EQ(milestone.getStartDate(), "2024-01-01");
  EXPECT_EQ(milestone.getEndDate(), "2024-02-01");
  EXPECT_EQ(milestone.getIssueIds().size(), 2U);
}

TEST(MilestoneTests, ConstructorRejectsMissingFields) {
  EXPECT_THROW(Milestone(-1, "", "desc", "2024-01-01", "2024-02-01"),
               std::invalid_argument);
  EXPECT_THROW(Milestone(-1, "name", "desc", "", "2024-02-01"),
               std::invalid_argument);
  EXPECT_THROW(Milestone(-1, "name", "desc", "2024-01-01", ""),
               std::invalid_argument);
}

TEST(MilestoneTests, DescriptionCanBeUpdated) {
  Milestone milestone(-1, "Sprint", "initial", "2024-01-01", "2024-02-01");
  milestone.setDescription("refined scope");
  EXPECT_EQ(milestone.getDescription(), "refined scope");
}

TEST(MilestoneTests, ScheduleUpdatesEnforceRequiredValues) {
  Milestone milestone(-1, "Sprint", "desc", "2024-01-01", "2024-02-01");
  milestone.setSchedule("2024-02-05", "2024-03-01");
  EXPECT_EQ(milestone.getStartDate(), "2024-02-05");
  EXPECT_EQ(milestone.getEndDate(), "2024-03-01");
  EXPECT_THROW(milestone.setStartDate(""), std::invalid_argument);
  EXPECT_THROW(milestone.setEndDate(""), std::invalid_argument);
}

TEST(MilestoneTests, PersistentIdCanOnlyBeSetOnce) {
  Milestone milestone(-1, "Sprint", "desc", "2024-01-01", "2024-02-01");
  milestone.setIdForPersistence(10);
  EXPECT_TRUE(milestone.hasPersistentId());
  EXPECT_EQ(milestone.getId(), 10);
  EXPECT_THROW(milestone.setIdForPersistence(11), std::logic_error);
}

TEST(MilestoneTests, AddIssueAvoidsDuplicates) {
  Milestone milestone(-1, "Sprint", "desc", "2024-01-01", "2024-02-01");
  milestone.addIssue(42);
  milestone.addIssue(42);
  milestone.addIssue(51);
  EXPECT_EQ(milestone.getIssueCount(), 2U);
  EXPECT_TRUE(milestone.hasIssue(42));
  EXPECT_TRUE(milestone.hasIssue(51));
  EXPECT_FALSE(milestone.hasIssue(100));
}

TEST(MilestoneTests, RemoveIssueIsIdempotent) {
  Milestone milestone(-1, "Sprint", "desc", "2024-01-01", "2024-02-01");
  milestone.addIssue(7);
  milestone.removeIssue(7);
  milestone.removeIssue(7);
  EXPECT_EQ(milestone.getIssueCount(), 0U);
}

TEST(MilestoneTests, ReplaceIssuesDeduplicatesIds) {
  Milestone milestone(-1, "Sprint", "desc", "2024-01-01", "2024-02-01");
  milestone.replaceIssues({5, 5, 7, 6, 7});
  const auto& ids = milestone.getIssueIds();
  EXPECT_EQ(ids.size(), 3U);
  EXPECT_EQ(ids[0], 5);
  EXPECT_EQ(ids[1], 6);
  EXPECT_EQ(ids[2], 7);
}
