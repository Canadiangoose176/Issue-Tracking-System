#include "Comment.hpp"
#include <utility>

// Construct and validate a comment (id >= -1, strings non-empty).
Comment::Comment(int id,
                 std::string author_id,
                 std::string text,
                 TimePoint timestamp)
    : id_{id},
      author_id_{std::move(author_id)},
      text_{std::move(text)},
      timestamp_{timestamp} {
  if (id_ < -1) throw std::invalid_argument("id must be >= -1");
  if (author_id_.empty()) throw std::invalid_argument("authorId empty");
  if (text_.empty()) throw std::invalid_argument("text empty");
}

// Whether this comment was persisted (id >= 0).
bool Comment::hasPersistentId() const noexcept { return id_ >= 0; }

// Current id value (0 if new).
int Comment::getId() const noexcept { return id_; }

// Assign a persistent id exactly once; must be >= 0.
void Comment::setIdForPersistence(int new_id) {
  if (hasPersistentId()) throw std::logic_error("id already set");
  if (new_id < 0) throw std::invalid_argument("new_id must be >= 0");
  id_ = new_id;
}

// Read accessors.
const std::string& Comment::getAuthor() const noexcept { return author_id_; }
void Comment::setAuthor(std::string author_id) {
  if (author_id.empty()) {
    throw std::invalid_argument("authorId empty");
  }
  author_id_ = std::move(author_id);
}
const std::string& Comment::getText() const noexcept { return text_; }
Comment::TimePoint Comment::getTimeStamp() const noexcept {
  return timestamp_;
}

// Replace text (must remain non-empty).
void Comment::setText(std::string new_text) {
  if (new_text.empty()) throw std::invalid_argument("text empty");
  text_ = std::move(new_text);
}

// Set timestamp (epoch ms).
void Comment::setTimeStamp(TimePoint ts) { timestamp_ = ts; }
