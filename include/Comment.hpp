#ifndef COMMENT_HPP_
#define COMMENT_HPP_

#include <cstdint>
#include <stdexcept>
#include <string>

/**
 * @brief Value object representing a single comment.
 *
 * Invariants:
 *  - New comments start with id == -1 (not persisted).
 *  - Description comments can use id == 0.
 *  - author_id_ and text_ must be non-empty.
 *  - timestamp_ is epoch ms; 0 means "unknown/unset".
 */
class Comment {
 public:
        /// @brief Epoch milliseconds (0 means unknown/unset).
        using TimePoint = std::int64_t;

 private:
        int id_{-1};             ///< -1 => new (not yet persisted)
        std::string author_id_;  ///< non-empty author user id
        std::string text_;       ///< non-empty comment text
        TimePoint timestamp_{0}; ///< creation/mod time; 0 => unknown

 public:
        /// @brief Default construct (id==0, empty fields).
        Comment() = default;

        /**
         * @brief Construct and validate a comment.
         * @param id         >= -1 (-1 new; 0 description; >0 persisted)
         * @param author_id  non-empty user id of author
         * @param text       non-empty body text
         * @param timestamp  epoch ms (0 allowed for unknown)
         * @throws std::invalid_argument on bad inputs
         */
        Comment(int id,
                std::string author_id,
                std::string text,
                TimePoint timestamp = 0);

        // ----------------- id helpers -----------------

        /**
         * @brief Whether this comment has a persistent id.
         * @return true if id_ >= 0, false otherwise.
         */
        bool hasPersistentId() const noexcept;

        /**
         * @brief Get the current id.
         * @return id value (0 if not yet persisted).
         */
        int getId() const noexcept;

        /**
         * @brief Assign a persistent id exactly once.
         * @param new_id  >= 0 (0 reserved for description)
         * @throws std::logic_error if id already set
         * @throws std::invalid_argument if new_id <= 0
         */
        void setIdForPersistence(int new_id);

        // ----------------- accessors ------------------

        /**
         * @brief Get the author user id.
         * @return reference to non-empty author id string.
         */
        const std::string &getAuthor() const noexcept;

        /**
         * @brief Set the author user id.
         * @param author_id non-empty user id
         * @throws std::invalid_argument if empty
         */
        void setAuthor(std::string author_id);

        /**
         * @brief Get the body text.
         * @return reference to non-empty text string.
         */
        const std::string &getText() const noexcept;

        /**
         * @brief Get timestamp.
         * @return epoch ms (0 if unknown).
         */
        TimePoint getTimeStamp() const noexcept;

        // ----------------- mutators -------------------

        /**
         * @brief Replace the body text.
         * @param new_text  non-empty text
         * @throws std::invalid_argument if new_text is empty
         */
        void setText(std::string new_text);

        /**
         * @brief Set timestamp value.
         * @param ts epoch ms (0 allowed for unknown)
         */
        void setTimeStamp(TimePoint ts);
};

#endif // COMMENT_HPP_
