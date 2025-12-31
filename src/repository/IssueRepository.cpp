#include "IssueRepository.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>

#include "SQLiteIssueRepository.hpp"

std::vector<Issue> IssueRepository::findIssues(
    const std::string& userId) const {
  return findIssues([&](const Issue& issue) {
    return issue.hasAssignee() && issue.getAssignedTo() == userId;
  });
}

std::vector<Issue> IssueRepository::listAllUnassigned() const {
  return findIssues([](const Issue& issue) { return !issue.hasAssignee(); });
}

bool IssueRepository::addTagToIssue(int issueId,
  const Tag& tag) {
  Issue issue = getIssue(issueId);
  bool added = issue.addTag(tag);
  saveIssue(issue);
  return added;
}

bool IssueRepository::removeTagFromIssue(int issueId,
  const std::string& tag) {
  Issue issue = getIssue(issueId);
  bool removed = issue.removeTag(tag);
  saveIssue(issue);
  return removed;
}

namespace {
std::string toLowerCopy(std::string value) {
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return value;
}
}  // namespace

class InMemoryIssueRepository : public SQLiteIssueRepository {
 public:
  InMemoryIssueRepository() : SQLiteIssueRepository(":memory:") {}
};

IssueRepository* createIssueRepository() {
  const char* backendEnv = std::getenv("ISSUE_REPO_BACKEND");
  std::string backend = backendEnv ? backendEnv : "";

  if (toLowerCopy(backend) == "memory") {
    return new InMemoryIssueRepository();
  }

  const char* dbPathEnv = std::getenv("ISSUE_DB_PATH");
  std::string dbPath = dbPathEnv ? dbPathEnv : "issues.db";
  return new SQLiteIssueRepository(dbPath);
}
