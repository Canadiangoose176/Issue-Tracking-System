#include "SQLiteIssueRepository.hpp"

#include <cctype>
#include <chrono>
#include <ctime>
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

class SqliteStmt {
 public:
  SqliteStmt(sqlite3* db, const std::string& sql) : stmt_(nullptr) {
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt_, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db));
    }
  }

  ~SqliteStmt() {
    if (stmt_ != nullptr) {
      sqlite3_finalize(stmt_);
      stmt_ = nullptr;
    }
  }

  sqlite3_stmt* get() const { return stmt_; }

  SqliteStmt(const SqliteStmt&) = delete;
  SqliteStmt& operator=(const SqliteStmt&) = delete;

 private:
  sqlite3_stmt* stmt_;
};

std::string columnText(sqlite3_stmt* stmt, int index);

std::string columnText(sqlite3_stmt* stmt, int index) {
  const unsigned char* text = sqlite3_column_text(stmt, index);
  return text ? reinterpret_cast<const char*>(text) : std::string();
}

Comment::TimePoint currentTimeMillis() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

class SqliteTxn {
 public:
  explicit SqliteTxn(sqlite3* db) : db_(db), active_(true) {
    if (sqlite3_exec(db_, "BEGIN IMMEDIATE;", nullptr, nullptr, nullptr) !=
        SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
  }

  ~SqliteTxn() {
    if (active_) {
      sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
  }

  void commit() {
    if (!active_) {
      return;
    }
    if (sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
      throw std::runtime_error(sqlite3_errmsg(db_));
    }
    active_ = false;
  }

 private:
  sqlite3* db_;
  bool active_;
};

int ensureTagId(sqlite3* db, const Tag& tag) {
  if (tag.getName().empty()) {
    throw std::invalid_argument("tag name must not be empty");
  }

  SqliteStmt findStmt(
      db, "SELECT id, color FROM tags WHERE LOWER(tag) = LOWER(?) LIMIT 1;");
  sqlite3_bind_text(findStmt.get(), 1, tag.getName().c_str(), -1,
                    SQLITE_TRANSIENT);
  int rc = sqlite3_step(findStmt.get());
  if (rc == SQLITE_ROW) {
    int id = sqlite3_column_int(findStmt.get(), 0);
    std::string currentColor = columnText(findStmt.get(), 1);
    if (!tag.getColor().empty() && tag.getColor() != currentColor) {
      SqliteStmt updateStmt(db, "UPDATE tags SET color = ? WHERE id = ?;");
      sqlite3_bind_text(updateStmt.get(), 1, tag.getColor().c_str(), -1,
                        SQLITE_TRANSIENT);
      sqlite3_bind_int(updateStmt.get(), 2, id);
      sqlite3_step(updateStmt.get());
    }
    return id;
  }

  SqliteStmt insertStmt(db, "INSERT INTO tags (tag, color) VALUES (?, ?);");
  sqlite3_bind_text(insertStmt.get(), 1, tag.getName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(insertStmt.get(), 2, tag.getColor().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_step(insertStmt.get());
  return static_cast<int>(sqlite3_last_insert_rowid(db));
}
}  // namespace

SQLiteIssueRepository::SQLiteIssueRepository(const std::string& dbPath)
    : db_(nullptr) {
  if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
    throw std::runtime_error("Failed to open SQLite database: " + dbPath);
  }
  execOrThrow("PRAGMA foreign_keys = ON;");
  initializeSchema();
}

SQLiteIssueRepository::~SQLiteIssueRepository() {
  if (db_ != nullptr) {
    sqlite3_close(db_);
    db_ = nullptr;
  }
}

void SQLiteIssueRepository::execOrThrow(const std::string& sql) const {
  char* errMsg = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
    std::string message = errMsg ? errMsg : "Unknown SQLite error";
    sqlite3_free(errMsg);
    throw std::runtime_error(message);
  }
}

void SQLiteIssueRepository::initializeSchema() {
  // Base schema. For a brand-new DB this will create the issues table
  // including the status column. For an existing DB, this has no effect.
  const char* statements[] = {
      "CREATE TABLE IF NOT EXISTS issues ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "author_id TEXT NOT NULL,"
      "title TEXT NOT NULL,"
      "description_comment_id INTEGER NOT NULL DEFAULT -1,"
      "assigned_to TEXT,"
      "created_at INTEGER DEFAULT 0);",

      "CREATE TABLE IF NOT EXISTS comments ("
      "id INTEGER NOT NULL,"
      "issue_id INTEGER NOT NULL,"
      "author_id TEXT NOT NULL,"
      "text TEXT NOT NULL,"
      "timestamp INTEGER DEFAULT 0,"
      "PRIMARY KEY(issue_id, id),"
      "FOREIGN KEY(issue_id) REFERENCES issues(id) ON DELETE CASCADE);",

      "CREATE TABLE IF NOT EXISTS tags ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "tag TEXT UNIQUE NOT NULL,"
      "color TEXT);",

      "CREATE TABLE IF NOT EXISTS users ("
      "name TEXT PRIMARY KEY,"
      "role TEXT NOT NULL);",

      "CREATE TABLE IF NOT EXISTS issue_tags ("
      "issue_id INTEGER NOT NULL,"
      "tag_id INTEGER NOT NULL,"
      "PRIMARY KEY(issue_id, tag_id),"
      "FOREIGN KEY(issue_id) REFERENCES issues(id) ON DELETE CASCADE,"
      "FOREIGN KEY(tag_id) REFERENCES tags(id) ON DELETE CASCADE);",

      "CREATE TABLE IF NOT EXISTS milestones ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
      "name TEXT NOT NULL,"
      "description TEXT,"
      "start_date TEXT NOT NULL,"
      "end_date TEXT NOT NULL);",

      "CREATE TABLE IF NOT EXISTS milestone_issues ("
      "milestone_id INTEGER NOT NULL,"
      "issue_id INTEGER NOT NULL,"
      "PRIMARY KEY(milestone_id, issue_id),"
      "FOREIGN KEY(milestone_id) REFERENCES milestones(id) ON DELETE CASCADE,"
      "FOREIGN KEY(issue_id) REFERENCES issues(id) ON DELETE CASCADE);",

      "CREATE INDEX IF NOT EXISTS idx_comments_issue ON comments(issue_id);"};

  for (const char* sql : statements) {
    execOrThrow(sql);
  }

  // Migration for older DBs that were created BEFORE the status column
  // existed. Older schema had no 'status' column, so we add it here.
  try {
    execOrThrow(
        "ALTER TABLE issues "
        "ADD COLUMN status TEXT NOT NULL "
        "DEFAULT 'To Be Done';");
  } catch (const std::runtime_error& e) {
    const std::string msg = e.what();
    // If the column already exists, SQLite will say something like
    // "duplicate column name: status". That is safe to ignore.
    if (msg.find("duplicate column name") == std::string::npos) {
      throw;
    }
  }
}

bool SQLiteIssueRepository::exists(
    const std::string& sql,
    const std::function<void(sqlite3_stmt*)>& binder) const {
  SqliteStmt stmt(db_, sql);
  if (binder) {
    binder(stmt.get());
  }
  int rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    return true;
  }
  if (rc == SQLITE_DONE) {
    return false;
  }
  throw std::runtime_error("Failed to execute exists query");
}

void SQLiteIssueRepository::forEachRow(
    const std::string& sql, const std::function<void(sqlite3_stmt*)>& binder,
    const std::function<void(sqlite3_stmt*)>& onRow) const {
  SqliteStmt stmt(db_, sql);
  if (binder) {
    binder(stmt.get());
  }

  while (true) {
    int rc = sqlite3_step(stmt.get());
    if (rc == SQLITE_ROW) {
      onRow(stmt.get());
    } else if (rc == SQLITE_DONE) {
      break;
    } else {
      throw std::runtime_error("Failed to read rows");
    }
  }
}

Comment SQLiteIssueRepository::insertCommentRow(int issueId,
                                                const Comment& comment,
                                                int commentId) {
  Comment stored = comment;
  if (stored.getTimeStamp() == 0) {
    stored.setTimeStamp(currentTimeMillis());
  }
  SqliteStmt stmt(
      db_,
      "INSERT INTO comments (id, issue_id, author_id, text, timestamp) "
      "VALUES (?, ?, ?, ?, ?);");
  sqlite3_bind_int(stmt.get(), 1, commentId);
  sqlite3_bind_int(stmt.get(), 2, issueId);
  sqlite3_bind_text(stmt.get(), 3, stored.getAuthor().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, stored.getText().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 5, stored.getTimeStamp());

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to insert comment");
  }

  if (!stored.hasPersistentId()) {
    stored.setIdForPersistence(commentId);
  }
  return stored;
}

bool SQLiteIssueRepository::issueExists(int issueId) const {
  return exists(
      "SELECT 1 FROM issues WHERE id = ? LIMIT 1;",
      [issueId](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, issueId); });
}

bool SQLiteIssueRepository::commentExists(int issueId, int commentId) const {
  return exists("SELECT 1 FROM comments WHERE issue_id = ? AND id = ? LIMIT 1;",
                [issueId, commentId](sqlite3_stmt* stmt) {
                  sqlite3_bind_int(stmt, 1, issueId);
                  sqlite3_bind_int(stmt, 2, commentId);
                });
}

int SQLiteIssueRepository::nextCommentIdForIssue(int issueId) const {
  SqliteStmt stmt(
      db_, "SELECT COALESCE(MAX(id), -1) FROM comments WHERE issue_id = ?;");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    throw std::runtime_error("Failed to compute next comment id");
  }
  int maxId = sqlite3_column_int(stmt.get(), 0);
  return maxId + 1;
}

std::vector<Comment> SQLiteIssueRepository::loadComments(int issueId) const {
  std::vector<Comment> comments;
  forEachRow(
      "SELECT id, author_id, text, timestamp FROM comments "
      "WHERE issue_id = ? ORDER BY id ASC;",
      [issueId](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, issueId); },
      [&comments](sqlite3_stmt* stmt) {
        comments.emplace_back(sqlite3_column_int(stmt, 0), columnText(stmt, 1),
                              columnText(stmt, 2),
                              sqlite3_column_int64(stmt, 3));
      });
  return comments;
}

Issue SQLiteIssueRepository::getIssue(int issueId) const {
  SqliteStmt stmt(
      db_,
      "SELECT id, author_id, title, description_comment_id, assigned_to, "
      "status, created_at "
      "FROM issues WHERE id = ? LIMIT 1;");
  sqlite3_bind_int(stmt.get(), 1, issueId);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    throw std::invalid_argument("Issue with given ID does not exist");
  }

  Issue issue(sqlite3_column_int(stmt.get(), 0), columnText(stmt.get(), 1),
              columnText(stmt.get(), 2), sqlite3_column_int64(stmt.get(), 6));

  const int descriptionId = sqlite3_column_int(stmt.get(), 3);
  const std::string assigned = columnText(stmt.get(), 4);
  const std::string status = columnText(stmt.get(), 5);

  if (!assigned.empty()) {
    issue.assignTo(assigned);
  }
  if (!status.empty()) {
    issue.setStatus(status);
  }

  for (const auto& comment : loadComments(issueId)) {
    issue.addComment(comment);
  }

  if (descriptionId >= 0 && issue.findCommentById(descriptionId) != nullptr) {
    issue.setDescriptionCommentId(descriptionId);
  }

  forEachRow(
      "SELECT t.id, t.tag, t.color "
      "FROM issue_tags it "
      "JOIN tags t ON t.id = it.tag_id "
      "WHERE it.issue_id = ?;",
      [issueId](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, issueId); },
      [&issue](sqlite3_stmt* stmt) {
        int tagId = sqlite3_column_int(stmt, 0);
        std::string tag = columnText(stmt, 1);
        std::string color = columnText(stmt, 2);
        if (!tag.empty()) {
          issue.addTag(Tag(tagId, tag, color));
        }
      });
  return issue;
}

Issue SQLiteIssueRepository::saveIssue(const Issue& issue) {
  Issue stored = issue;

  // ---- INSERT NEW ISSUE ----
  if (!stored.hasPersistentId()) {
    if (stored.getTimestamp() == 0) {
      stored.setTimestamp(currentTimeMillis());
    }

    SqliteStmt insertStmt(
        db_,
        "INSERT INTO issues (author_id, title, description_comment_id, "
        "assigned_to, status, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?);");

    sqlite3_bind_text(insertStmt.get(), 1, stored.getAuthorId().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(insertStmt.get(), 2, stored.getTitle().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(insertStmt.get(), 3, stored.getDescriptionCommentId());

    if (stored.hasAssignee()) {
      sqlite3_bind_text(insertStmt.get(), 4, stored.getAssignedTo().c_str(), -1,
                        SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_null(insertStmt.get(), 4);
    }

    sqlite3_bind_text(insertStmt.get(), 5, stored.getStatus().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int64(insertStmt.get(), 6, stored.getTimestamp());

    if (sqlite3_step(insertStmt.get()) != SQLITE_DONE) {
      throw std::runtime_error("Failed to insert issue");
    }

    int newId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    stored.setIdForPersistence(newId);

    return getIssue(newId);
  }

  // ---- UPDATE EXISTING ISSUE ----
  if (!issueExists(stored.getId())) {
    throw std::invalid_argument("Issue with given ID does not exist: " +
                                std::to_string(stored.getId()));
  }

  if (stored.getTimestamp() == 0) {
    stored.setTimestamp(currentTimeMillis());
  }

  {
    SqliteStmt updateStmt(db_,
                          "UPDATE issues SET author_id = ?, title = ?, "
                          "description_comment_id = ?, assigned_to = ?, "
                          "status = ?, created_at = ? WHERE id = ?;");

    sqlite3_bind_text(updateStmt.get(), 1, stored.getAuthorId().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(updateStmt.get(), 2, stored.getTitle().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int(updateStmt.get(), 3, stored.getDescriptionCommentId());

    if (stored.hasAssignee()) {
      sqlite3_bind_text(updateStmt.get(), 4, stored.getAssignedTo().c_str(), -1,
                        SQLITE_TRANSIENT);
    } else {
      sqlite3_bind_null(updateStmt.get(), 4);
    }

    sqlite3_bind_text(updateStmt.get(), 5, stored.getStatus().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_int64(updateStmt.get(), 6, stored.getTimestamp());
    sqlite3_bind_int(updateStmt.get(), 7, stored.getId());

    if (sqlite3_step(updateStmt.get()) != SQLITE_DONE) {
      throw std::runtime_error("Failed to update issue");
    }
  }

  // ---- DELETE OLD TAGS ----
  {
    SqliteStmt deleteStmt(db_, "DELETE FROM issue_tags WHERE issue_id = ?;");
    sqlite3_bind_int(deleteStmt.get(), 1, stored.getId());
    sqlite3_step(deleteStmt.get());
  }

  // ---- INSERT NEW TAGS ----
  for (const auto& tag : stored.getTags()) {
    int tagId = ensureTagId(db_, tag);
    SqliteStmt tagStmt(
        db_,
        "INSERT OR IGNORE INTO issue_tags (issue_id, tag_id) VALUES (?, ?);");
    sqlite3_bind_int(tagStmt.get(), 1, stored.getId());
    sqlite3_bind_int(tagStmt.get(), 2, tagId);
    sqlite3_step(tagStmt.get());
  }

  return getIssue(stored.getId());
}

bool SQLiteIssueRepository::deleteIssue(int issueId) {
  SqliteStmt stmt(db_, "DELETE FROM issues WHERE id = ?;");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to delete issue");
  }
  return sqlite3_changes(db_) > 0;
}

std::vector<Issue> SQLiteIssueRepository::listIssues() const {
  std::vector<Issue> issues;
  forEachRow("SELECT id FROM issues ORDER BY id ASC;", nullptr,
             [this, &issues](sqlite3_stmt* stmt) {
               int id = sqlite3_column_int(stmt, 0);
               issues.push_back(getIssue(id));
             });
  return issues;
}

// Generic filter used by specific find/list methods.
std::vector<Issue> SQLiteIssueRepository::findIssues(
    std::function<bool(const Issue&)> criteria) const {
  std::vector<Issue> all = listIssues();
  std::vector<Issue> filtered;
  for (Issue& issue : all) {
    if (criteria(issue)) {
      filtered.push_back(issue);
    }
  }
  return filtered;
}

std::vector<Tag> SQLiteIssueRepository::listAllTags() const {
  std::vector<Tag> tags;
  forEachRow("SELECT id, tag, color FROM tags ORDER BY tag ASC;", nullptr,
             [&tags](sqlite3_stmt* stmt) {
               tags.emplace_back(sqlite3_column_int(stmt, 0),
                                 columnText(stmt, 1), columnText(stmt, 2));
             });
  return tags;
}

bool SQLiteIssueRepository::deleteTag(int tagId) {
  if (tagId <= 0) {
    return false;
  }

  SqliteStmt deleteLinks(db_, "DELETE FROM issue_tags WHERE tag_id = ?;");
  sqlite3_bind_int(deleteLinks.get(), 1, tagId);
  sqlite3_step(deleteLinks.get());

  SqliteStmt stmt(db_, "DELETE FROM tags WHERE id = ?;");
  sqlite3_bind_int(stmt.get(), 1, tagId);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to delete tag definition");
  }
  return sqlite3_changes(db_) > 0;
}

bool SQLiteIssueRepository::updateTagDefinition(int tagId, const Tag& tag) {
  if (tagId <= 0) {
    return false;
  }

  SqliteStmt findStmt(db_, "SELECT tag, color FROM tags WHERE id = ? LIMIT 1;");
  sqlite3_bind_int(findStmt.get(), 1, tagId);
  if (sqlite3_step(findStmt.get()) != SQLITE_ROW) {
    return false;
  }
  std::string currentName = columnText(findStmt.get(), 0);
  std::string currentColor = columnText(findStmt.get(), 1);

  const std::string& nextName =
      tag.getName().empty() ? currentName : tag.getName();
  const std::string& nextColor =
      tag.getColor().empty() ? currentColor : tag.getColor();

  SqliteStmt updateStmt(db_,
                        "UPDATE tags SET tag = ?, color = ? WHERE id = ?;");
  sqlite3_bind_text(updateStmt.get(), 1, nextName.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(updateStmt.get(), 2, nextColor.c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(updateStmt.get(), 3, tagId);
  int rc = sqlite3_step(updateStmt.get());
  if (rc != SQLITE_DONE) {
    throw std::runtime_error("Failed to update tag");
  }
  return true;
}

// --- Interface overrides that your controller uses ---

std::vector<Issue> SQLiteIssueRepository::findIssues(
    const std::string& userId) const {
  // Same semantics as in-memory repo: match author or assignee.
  return findIssues([&userId](const Issue& issue) {
    if (issue.getAuthorId() == userId) {
      return true;
    }
    return issue.hasAssignee() && issue.getAssignedTo() == userId;
  });
}

std::vector<Issue> SQLiteIssueRepository::listAllUnassigned() const {
  return findIssues([](const Issue& issue) { return !issue.hasAssignee(); });
}

bool SQLiteIssueRepository::addTagToIssue(int issueId, const Tag& tag) {
  if (tag.getName().empty() || !issueExists(issueId)) {
    return false;
  }

  int tagId = ensureTagId(db_, tag);

  bool alreadyAttached = exists(
      "SELECT 1 FROM issue_tags "
      "WHERE issue_id = ? "
      "AND tag_id = ? "
      "LIMIT 1;",

      [issueId, tagId](sqlite3_stmt* stmt) {
        sqlite3_bind_int(stmt, 1, issueId);
        sqlite3_bind_int(stmt, 2, tagId);
      });

  if (alreadyAttached) {
    return true;
  }

  SqliteStmt stmt(db_,
                  "INSERT INTO issue_tags (issue_id, tag_id) VALUES (?, ?);");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  sqlite3_bind_int(stmt.get(), 2, tagId);
  sqlite3_step(stmt.get());
  return true;
}

bool SQLiteIssueRepository::removeTagFromIssue(int issueId,
                                               const std::string& tag) {
  if (tag.empty() || !issueExists(issueId)) {
    return false;
  }

  SqliteStmt findStmt(
      db_, "SELECT id FROM tags WHERE LOWER(tag) = LOWER(?) LIMIT 1;");
  sqlite3_bind_text(findStmt.get(), 1, tag.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(findStmt.get()) != SQLITE_ROW) {
    return false;
  }
  int tagId = sqlite3_column_int(findStmt.get(), 0);

  SqliteStmt stmt(db_,
                  "DELETE FROM issue_tags "
                  "WHERE issue_id = ? "
                  "AND tag_id = ?;");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  sqlite3_bind_int(stmt.get(), 2, tagId);
  sqlite3_step(stmt.get());
  return sqlite3_changes(db_) > 0;
}

bool SQLiteIssueRepository::updateTagOnIssue(int issueId,
                                             const std::string& currentTag,
                                             const Tag& updatedTag) {
  if (!issueExists(issueId) || currentTag.empty() ||
      updatedTag.getName().empty()) {
    return false;
  }

  SqliteStmt findStmt(
      db_, "SELECT id FROM tags WHERE LOWER(tag) = LOWER(?) LIMIT 1;");
  sqlite3_bind_text(findStmt.get(), 1, currentTag.c_str(), -1,
                    SQLITE_TRANSIENT);
  if (sqlite3_step(findStmt.get()) != SQLITE_ROW) {
    return false;
  }
  int currentId = sqlite3_column_int(findStmt.get(), 0);

  int targetId = ensureTagId(db_, updatedTag);
  if (targetId == currentId) {
    return true;
  }

  SqliteStmt updateStmt(
      db_,
      "UPDATE issue_tags SET tag_id = ? WHERE issue_id = ? AND tag_id = ?;");
  sqlite3_bind_int(updateStmt.get(), 1, targetId);
  sqlite3_bind_int(updateStmt.get(), 2, issueId);
  sqlite3_bind_int(updateStmt.get(), 3, currentId);
  sqlite3_step(updateStmt.get());
  return sqlite3_changes(db_) > 0;
}

// --- Comments ---

Comment SQLiteIssueRepository::getComment(int issueId, int commentId) const {
  SqliteStmt stmt(db_,
                  "SELECT id, author_id, text, timestamp FROM comments "
                  "WHERE issue_id = ? AND id = ? LIMIT 1;");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  sqlite3_bind_int(stmt.get(), 2, commentId);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    throw std::invalid_argument("Comment does not belong to the given issue");
  }

  int id = sqlite3_column_int(stmt.get(), 0);
  std::string author = columnText(stmt.get(), 1);
  std::string text = columnText(stmt.get(), 2);
  Comment::TimePoint ts = sqlite3_column_int64(stmt.get(), 3);
  return Comment(id, author, text, ts);
}

std::vector<Comment> SQLiteIssueRepository::getAllComments(int issueId) const {
  if (!issueExists(issueId)) {
    throw std::invalid_argument("Issue with given ID does not exist");
  }
  return loadComments(issueId);
}

Comment SQLiteIssueRepository::saveComment(int issueId,
                                           const Comment& comment) {
  if (!issueExists(issueId)) {
    throw std::invalid_argument("Issue with given ID does not exist");
  }

  if (!comment.hasPersistentId()) {
    const int newId = nextCommentIdForIssue(issueId);
    return insertCommentRow(issueId, comment, newId);
  }

  const int commentId = comment.getId();
  if (!commentExists(issueId, commentId)) {
    if (commentId == 0) {
      return insertCommentRow(issueId, comment, 0);
    }
    throw std::invalid_argument("Comment with given ID does not exist");
  }

  Comment updated = comment;

  SqliteStmt stmt(db_,
                  "UPDATE comments SET author_id = ?, text = ?, timestamp = ? "
                  "WHERE issue_id = ? AND id = ?;");

  sqlite3_bind_text(stmt.get(), 1, updated.getAuthor().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, updated.getText().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int64(stmt.get(), 3, updated.getTimeStamp());
  sqlite3_bind_int(stmt.get(), 4, issueId);
  sqlite3_bind_int(stmt.get(), 5, commentId);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to update comment");
  }

  return updated;
}

bool SQLiteIssueRepository::deleteComment(int issueId, int commentId) {
  if (!issueExists(issueId)) {
    throw std::invalid_argument("Issue with given ID does not exist");
  }

  if (!commentExists(issueId, commentId)) {
    throw std::invalid_argument("Comment with given ID does not exist");
  }

  SqliteStmt clearDesc(
      db_,
      "UPDATE issues SET description_comment_id = -1 WHERE id = ? "
      "AND description_comment_id = ?;");
  sqlite3_bind_int(clearDesc.get(), 1, issueId);
  sqlite3_bind_int(clearDesc.get(), 2, commentId);
  sqlite3_step(clearDesc.get());

  SqliteStmt stmt(db_, "DELETE FROM comments WHERE issue_id = ? AND id = ?;");
  sqlite3_bind_int(stmt.get(), 1, issueId);
  sqlite3_bind_int(stmt.get(), 2, commentId);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to delete comment");
  }
  return sqlite3_changes(db_) > 0;
}

// --- Users ---

User SQLiteIssueRepository::getUser(const std::string& userId) const {
  SqliteStmt stmt(db_, "SELECT name, role FROM users WHERE name = ? LIMIT 1;");
  sqlite3_bind_text(stmt.get(), 1, userId.c_str(), -1, SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    throw std::invalid_argument("User with given ID does not exist");
  }

  std::string name = columnText(stmt.get(), 0);
  std::string role = columnText(stmt.get(), 1);
  return User(name, role);
}

User SQLiteIssueRepository::saveUser(const User& user) {
  if (user.getName().empty()) {
    throw std::invalid_argument("User ID must be non-empty");
  }

  SqliteStmt stmt(db_,
                  "INSERT INTO users (name, role) VALUES (?, ?) "
                  "ON CONFLICT(name) DO UPDATE SET role = excluded.role;");
  sqlite3_bind_text(stmt.get(), 1, user.getName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, user.getRole().c_str(), -1,
                    SQLITE_TRANSIENT);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to save user");
  }
  return user;
}

bool SQLiteIssueRepository::deleteUser(const std::string& userId) {
  SqliteStmt stmt(db_, "DELETE FROM users WHERE name = ?;");
  sqlite3_bind_text(stmt.get(), 1, userId.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to delete user");
  }
  return sqlite3_changes(db_) > 0;
}

std::vector<User> SQLiteIssueRepository::listAllUsers() const {
  std::vector<User> users;
  forEachRow("SELECT name, role FROM users ORDER BY name ASC;", nullptr,
             [&users](sqlite3_stmt* stmt) {
               users.emplace_back(columnText(stmt, 0), columnText(stmt, 1));
             });
  return users;
}

// ==================== MILESTONE SQL IMPLEMENTATION ====================

std::vector<int> SQLiteIssueRepository::loadMilestoneIssueIds(
    int milestoneId) const {
  std::vector<int> ids;
  forEachRow(
      "SELECT issue_id FROM milestone_issues WHERE milestone_id = ? "
      "ORDER BY issue_id ASC;",
      [milestoneId](sqlite3_stmt* stmt) {
        sqlite3_bind_int(stmt, 1, milestoneId);
      },
      [&ids](sqlite3_stmt* stmt) {
        ids.push_back(sqlite3_column_int(stmt, 0));
      });
  return ids;
}

bool SQLiteIssueRepository::milestoneExists(int milestoneId) const {
  SqliteStmt stmt(db_, "SELECT 1 FROM milestones WHERE id = ? LIMIT 1;");
  sqlite3_bind_int(stmt.get(), 1, milestoneId);
  int rc = sqlite3_step(stmt.get());
  if (rc == SQLITE_ROW) {
    return true;
  }
  if (rc == SQLITE_DONE) {
    return false;
  }
  throw std::runtime_error("Failed to verify milestone existence");
}

Milestone SQLiteIssueRepository::saveMilestone(const Milestone& milestone) {
  if (milestone.getName().empty() || milestone.getStartDate().empty() ||
      milestone.getEndDate().empty()) {
    throw std::invalid_argument("Milestone requires name/start/end dates");
  }

  if (!milestone.hasPersistentId()) {
    SqliteStmt stmt(
        db_,
        "INSERT INTO milestones (name, description, start_date, end_date) "
        "VALUES (?, ?, ?, ?);");
    sqlite3_bind_text(stmt.get(), 1, milestone.getName().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 2, milestone.getDescription().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 3, milestone.getStartDate().c_str(), -1,
                      SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt.get(), 4, milestone.getEndDate().c_str(), -1,
                      SQLITE_TRANSIENT);

    if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
      throw std::runtime_error("Failed to insert milestone");
    }
    int newId = static_cast<int>(sqlite3_last_insert_rowid(db_));
    return getMilestone(newId);
  }

  if (!milestoneExists(milestone.getId())) {
    throw std::out_of_range("Milestone not found");
  }

  SqliteStmt stmt(
      db_,
      "UPDATE milestones SET name = ?, description = ?, start_date = ?, "
      "end_date = ? WHERE id = ?;");
  sqlite3_bind_text(stmt.get(), 1, milestone.getName().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 2, milestone.getDescription().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 3, milestone.getStartDate().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_text(stmt.get(), 4, milestone.getEndDate().c_str(), -1,
                    SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt.get(), 5, milestone.getId());

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to update milestone");
  }
  return getMilestone(milestone.getId());
}

Milestone SQLiteIssueRepository::getMilestone(int milestoneId) const {
  SqliteStmt stmt(db_,
                  "SELECT id, name, description, start_date, end_date "
                  "FROM milestones WHERE id = ?;");
  sqlite3_bind_int(stmt.get(), 1, milestoneId);

  if (sqlite3_step(stmt.get()) != SQLITE_ROW) {
    throw std::out_of_range("Milestone not found");
  }

  int id = sqlite3_column_int(stmt.get(), 0);
  std::string name = columnText(stmt.get(), 1);
  std::string desc = columnText(stmt.get(), 2);
  std::string start = columnText(stmt.get(), 3);
  std::string end = columnText(stmt.get(), 4);
  std::vector<int> issueIds = loadMilestoneIssueIds(id);

  return Milestone(id, name, desc, start, end, issueIds);
}

bool SQLiteIssueRepository::deleteMilestone(int milestoneId, bool cascade) {
  if (!milestoneExists(milestoneId)) {
    throw std::out_of_range("Milestone not found");
  }

  SqliteTxn txn(db_);
  if (cascade) {
    std::vector<int> issueIds = loadMilestoneIssueIds(milestoneId);
    for (int issueId : issueIds) {
      deleteIssue(issueId);
    }
  }

  SqliteStmt stmt(db_, "DELETE FROM milestones WHERE id = ?;");
  sqlite3_bind_int(stmt.get(), 1, milestoneId);
  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to delete milestone");
  }
  bool removed = sqlite3_changes(db_) > 0;
  txn.commit();
  return removed;
}

std::vector<Milestone> SQLiteIssueRepository::listAllMilestones() const {
  std::vector<Milestone> list;
  forEachRow(
      "SELECT id, name, description, start_date, end_date FROM milestones "
      "ORDER BY start_date ASC, id ASC;",
      {}, [this, &list](sqlite3_stmt* stmt) {
        int id = sqlite3_column_int(stmt, 0);
        std::string name = columnText(stmt, 1);
        std::string desc = columnText(stmt, 2);
        std::string start = columnText(stmt, 3);
        std::string end = columnText(stmt, 4);
        list.emplace_back(id, name, desc, start, end,
                          loadMilestoneIssueIds(id));
      });
  return list;
}

bool SQLiteIssueRepository::addIssueToMilestone(int milestoneId, int issueId) {
  if (!milestoneExists(milestoneId)) {
    throw std::out_of_range("Milestone not found");
  }
  if (!issueExists(issueId)) {
    throw std::invalid_argument("Issue with given ID does not exist");
  }

  SqliteStmt stmt(
      db_,
      "INSERT OR IGNORE INTO milestone_issues (milestone_id, issue_id) "
      "VALUES (?, ?);");
  sqlite3_bind_int(stmt.get(), 1, milestoneId);
  sqlite3_bind_int(stmt.get(), 2, issueId);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to link issue to milestone");
  }
  return sqlite3_changes(db_) > 0;
}

bool SQLiteIssueRepository::removeIssueFromMilestone(int milestoneId,
                                                     int issueId) {
  if (!milestoneExists(milestoneId)) {
    throw std::out_of_range("Milestone not found");
  }

  SqliteStmt stmt(
      db_,
      "DELETE FROM milestone_issues WHERE milestone_id = ? AND issue_id = ?;");
  sqlite3_bind_int(stmt.get(), 1, milestoneId);
  sqlite3_bind_int(stmt.get(), 2, issueId);

  if (sqlite3_step(stmt.get()) != SQLITE_DONE) {
    throw std::runtime_error("Failed to unlink issue from milestone");
  }
  return sqlite3_changes(db_) > 0;
}

std::vector<Issue> SQLiteIssueRepository::getIssuesForMilestone(
    int milestoneId) const {
  if (!milestoneExists(milestoneId)) {
    throw std::out_of_range("Milestone not found");
  }

  std::vector<Issue> issues;
  forEachRow(
      "SELECT issue_id FROM milestone_issues WHERE milestone_id = ?;",
      [milestoneId](sqlite3_stmt* stmt) {
        sqlite3_bind_int(stmt, 1, milestoneId);
      },
      [this, &issues](sqlite3_stmt* stmt) {
        int issueId = sqlite3_column_int(stmt, 0);
        issues.push_back(getIssue(issueId));
      });
  return issues;
}
