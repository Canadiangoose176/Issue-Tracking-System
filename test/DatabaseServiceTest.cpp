#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "service/DatabaseService.hpp"

using ::testing::ElementsAre;

namespace {

class EnvVarGuard {
 public:
  EnvVarGuard(const std::string& key, const std::string& value) : key_(key) {
    const char* existing = std::getenv(key.c_str());
    if (existing) {
      original_ = std::string(existing);
    }
    setenv(key.c_str(), value.c_str(), 1);
  }

  EnvVarGuard(const EnvVarGuard&) = delete;
  EnvVarGuard& operator=(const EnvVarGuard&) = delete;

  ~EnvVarGuard() {
    if (original_) {
      setenv(key_.c_str(), original_->c_str(), 1);
    } else {
      unsetenv(key_.c_str());
    }
  }

 private:
  std::string key_;
  std::optional<std::string> original_;
};

std::filesystem::path makeTempRoot() {
  return std::filesystem::temp_directory_path() /
         ("dbservice-" +
          std::to_string(
              std::chrono::steady_clock::now().time_since_epoch().count()));
}

class TempDirCleaner {
 public:
  explicit TempDirCleaner(std::filesystem::path root)
      : root_(std::move(root)) {}

  TempDirCleaner(const TempDirCleaner&) = delete;
  TempDirCleaner& operator=(const TempDirCleaner&) = delete;

  ~TempDirCleaner() {
    std::error_code ec;
    std::filesystem::remove_all(root_, ec);
  }

 private:
  std::filesystem::path root_;
};

}  // namespace

TEST(DatabaseServiceTest, MemoryBackendShortCircuitsDiskOperations) {
  EnvVarGuard backend("ISSUE_REPO_BACKEND", "memory");
  EnvVarGuard dbPath("ISSUE_DB_PATH", "ignored.db");

  DatabaseService service;

  EXPECT_EQ(service.listDatabases(), std::vector<std::string>{":memory:"});
  EXPECT_EQ(service.getActiveDatabaseName(), ":memory:");
  EXPECT_FALSE(service.createDatabase("newdb"));
  EXPECT_FALSE(service.deleteDatabase("newdb"));
  EXPECT_FALSE(service.switchDatabase("anything"));
}

TEST(DatabaseServiceTest, RespectsCustomPathAndCreatesDirectory) {
  const auto tempRoot = makeTempRoot();
  const auto initialPath = tempRoot / "nested" / "initial.db";
  TempDirCleaner cleanup(tempRoot);

  EnvVarGuard backend("ISSUE_REPO_BACKEND", "sqlite");
  EnvVarGuard dbPath("ISSUE_DB_PATH", initialPath.string());

  ASSERT_FALSE(std::filesystem::exists(initialPath.parent_path()));

  {
    DatabaseService service;

    EXPECT_EQ(service.getActiveDatabaseName(), "initial.db");
    EXPECT_TRUE(std::filesystem::exists(initialPath.parent_path()));
    EXPECT_TRUE(std::filesystem::exists(initialPath));
  }
}

TEST(DatabaseServiceTest, CreateListSwitchAndDeleteDatabases) {
  const auto tempRoot = makeTempRoot();
  const auto basePath = tempRoot / "base.db";
  TempDirCleaner cleanup(tempRoot);

  EnvVarGuard backend("ISSUE_REPO_BACKEND", "sqlite");
  EnvVarGuard dbPath("ISSUE_DB_PATH", basePath.string());

  DatabaseService service;

  ASSERT_TRUE(std::filesystem::exists(basePath));
  EXPECT_FALSE(service.createDatabase(".."));

  EXPECT_TRUE(service.createDatabase("alpha"));
  EXPECT_FALSE(service.createDatabase("alpha"));

  auto dbs = service.listDatabases();
  EXPECT_THAT(dbs, ElementsAre("alpha.db", "base.db"));

  EXPECT_TRUE(service.switchDatabase("alpha"));
  EXPECT_EQ(service.getActiveDatabaseName(), "alpha.db");
  EXPECT_FALSE(service.deleteDatabase("alpha"));
  EXPECT_FALSE(service.deleteDatabase("missing"));

  EXPECT_TRUE(service.switchDatabase("base.db"));
  EXPECT_TRUE(service.deleteDatabase("alpha"));
  auto afterDelete = service.listDatabases();
  EXPECT_THAT(afterDelete, ElementsAre("base.db"));
}
