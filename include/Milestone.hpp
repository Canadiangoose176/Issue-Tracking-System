#ifndef MILESTONE_HPP
#define MILESTONE_HPP

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

/**
 * @brief Aggregate representing a milestone that can track multiple issues.
 *
 * The object enforces a minimal set of invariants:
 *  - name/start/end dates are required and non-empty.
 *  - persisted milestones must have id >= 0; transient milestones use -1.
 *  - issue ids are unique within the milestone.
 */
class Milestone {
 private:
  int id_{-1};
  std::string name_;
  std::string description_;
  std::string start_date_;
  std::string end_date_;
  std::vector<int> issue_ids_;

  static void validateRequiredField(const std::string& value,
                                    const char* field);
  static void validateIssueId(int issueId);

 public:
  Milestone() = default;
  Milestone(int id,
            std::string name,
            std::string description,
            std::string startDate,
            std::string endDate,
            std::vector<int> issueIds = {});

  bool hasPersistentId() const noexcept { return id_ >= 0; }
  int getId() const noexcept { return id_; }
  void setIdForPersistence(int id);

  const std::string& getName() const noexcept { return name_; }
  const std::string& getDescription() const noexcept { return description_; }
  const std::string& getStartDate() const noexcept { return start_date_; }
  const std::string& getEndDate() const noexcept { return end_date_; }
  const std::vector<int>& getIssueIds() const noexcept { return issue_ids_; }

  void setName(std::string name);
  void setDescription(std::string description) noexcept;
  void setStartDate(std::string startDate);
  void setEndDate(std::string endDate);
  void setSchedule(std::string startDate, std::string endDate);

  void replaceIssues(std::vector<int> issueIds);
  void addIssue(int issueId);
  void removeIssue(int issueId);
  bool hasIssue(int issueId) const noexcept;
  std::size_t getIssueCount() const noexcept { return issue_ids_.size(); }
};

#endif
