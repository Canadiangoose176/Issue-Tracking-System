#ifndef DATABASE_SERVICE_HPP_
#define DATABASE_SERVICE_HPP_

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "IssueService.hpp"
#include "SQLiteIssueRepository.hpp"

class DatabaseService {
 private:
 bool useMemoryBackend_;
  std::string activeDbPath_;
  std::string dbDirectory_;
  std::unique_ptr<IssueService> issueService_;

  static bool isMemoryBackendConfigured() {
    const char* backendEnv = std::getenv("ISSUE_REPO_BACKEND");
    if (!backendEnv) {
      return false;
    }
    std::string backend = backendEnv;
    std::transform(backend.begin(), backend.end(), backend.begin(),
                   [](unsigned char c) {
                     return static_cast<char>(std::tolower(c));
                   });
    return backend == "memory";
  }

  static std::string defaultDbPath() {
    const char* dbPathEnv = std::getenv("ISSUE_DB_PATH");
    return dbPathEnv ? std::string(dbPathEnv) : std::string("issues.db");
  }

  std::string resolveInitialDbPath() const {
    if (useMemoryBackend_) {
      return ":memory:";
    }
    return defaultDbPath();
  }

  static std::string resolveDirectory(const std::string& path) {
    std::filesystem::path fsPath(path);
    std::string dir = fsPath.parent_path().string();
    return dir.empty() ? std::string(".") : dir;
  }

  void ensureDbDirectoryExists() const {
    if (useMemoryBackend_) {
      return;
    }
    if (!std::filesystem::exists(dbDirectory_)) {
      std::filesystem::create_directories(dbDirectory_);
    }
  }

  static bool isValidDbName(const std::string& name) {
    if (name.empty()) {
      return false;
    }
    if (name.find('/') != std::string::npos ||
        name.find('\\') != std::string::npos ||
        name.find("..") != std::string::npos) {
      return false;
    }
    return std::all_of(name.begin(), name.end(),
                       [](unsigned char c) {
                         return std::isalnum(c) || c == '_' ||
                                c == '-' || c == '.';
                       });
  }

  std::string nameWithExtension(const std::string& name) const {
    if (name.size() >= 3 &&
        name.substr(name.size() - 3) == ".db") {
      return name;
    }
    return name + ".db";
  }

  std::string databasePathForName(const std::string& name) const {
    if (!isValidDbName(name)) {
      return "";
    }
    std::filesystem::path path(dbDirectory_);
    path /= nameWithExtension(name);
    return path.string();
  }

  std::unique_ptr<IssueRepository> buildRepository(
      const std::string& dbPath) const {
    if (useMemoryBackend_) {
      return std::make_unique<SQLiteIssueRepository>(":memory:");
    }
    return std::make_unique<SQLiteIssueRepository>(dbPath);
  }

  std::unique_ptr<IssueService> buildIssueService(
      const std::string& dbPath) const {
    return std::make_unique<IssueService>(buildRepository(dbPath));
  }

  void resetIssueService(const std::string& dbPath) {
    issueService_ = buildIssueService(dbPath);
    activeDbPath_ = dbPath;
  }

 public:
  DatabaseService()
      : useMemoryBackend_(isMemoryBackendConfigured()),
        activeDbPath_(resolveInitialDbPath()),
        dbDirectory_(resolveDirectory(activeDbPath_)) {
    ensureDbDirectoryExists();
    issueService_ = buildIssueService(activeDbPath_);
  }

  IssueService& getIssueService() const { return *issueService_; }

  std::vector<std::string> listDatabases() const {
    if (useMemoryBackend_) {
      return {":memory:"};
    }
    std::vector<std::string> dbs;
    if (!std::filesystem::exists(dbDirectory_)) {
      return dbs;
    }
    for (const auto& entry :
         std::filesystem::directory_iterator(dbDirectory_)) {
      if (entry.is_regular_file() &&
          entry.path().extension() == ".db") {
        dbs.push_back(entry.path().filename().string());
      }
    }
    std::sort(dbs.begin(), dbs.end());
    dbs.erase(std::unique(dbs.begin(), dbs.end()), dbs.end());
    return dbs;
  }

  std::string getActiveDatabaseName() const {
    if (useMemoryBackend_) {
      return ":memory:";
    }
    return std::filesystem::path(activeDbPath_).filename().string();
  }

  bool createDatabase(const std::string& name) {
    if (useMemoryBackend_) {
      return false;
    }
    const std::string target = databasePathForName(name);
    if (target.empty() || std::filesystem::exists(target)) {
      return false;
    }
    ensureDbDirectoryExists();
    auto tmpRepo = buildRepository(target);
    (void)tmpRepo;
    return std::filesystem::exists(target);
  }

  bool deleteDatabase(const std::string& name) {
    if (useMemoryBackend_) {
      return false;
    }
    const std::string target = databasePathForName(name);
    if (target.empty()) {
      return false;
    }

    const auto normalizedActive =
        std::filesystem::absolute(std::filesystem::path(activeDbPath_));
    const auto normalizedTarget =
        std::filesystem::absolute(std::filesystem::path(target));
    if (normalizedActive == normalizedTarget) {
      return false;
    }

    if (!std::filesystem::exists(target)) {
      return false;
    }
    return std::filesystem::remove(target);
  }

  bool switchDatabase(const std::string& name) {
    if (useMemoryBackend_) {
      return false;
    }
    const std::string target = databasePathForName(name);
    if (target.empty() || !std::filesystem::exists(target)) {
      return false;
    }
    resetIssueService(target);
    return true;
  }

  bool renameDatabase(const std::string& currentName,
                      const std::string& newName) {
    if (useMemoryBackend_) {
      return false;
    }

    const std::string sourcePath = databasePathForName(currentName);
    const std::string targetPath = databasePathForName(newName);
    if (sourcePath.empty() || targetPath.empty()) {
      return false;
    }

    if (!std::filesystem::exists(sourcePath)) {
      return false;
    }
    if (sourcePath == targetPath) {
      return true;
    }
    if (std::filesystem::exists(targetPath)) {
      return false;
    }

    std::error_code ec;
    std::filesystem::rename(sourcePath, targetPath, ec);
    if (ec) {
      return false;
    }

    const auto normalizedActive =
        std::filesystem::absolute(std::filesystem::path(activeDbPath_));
    const auto normalizedTarget =
        std::filesystem::absolute(std::filesystem::path(targetPath));
    if (normalizedActive ==
        std::filesystem::absolute(std::filesystem::path(sourcePath))) {
      resetIssueService(normalizedTarget.string());
    }

    return true;
  }
};

#endif
