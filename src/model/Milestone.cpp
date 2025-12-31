#include "Milestone.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace {
void ensureUnique(std::vector<int>* ids) {
  std::sort(ids->begin(), ids->end());
  ids->erase(std::unique(ids->begin(), ids->end()), ids->end());
}
}  // namespace

Milestone::Milestone(int id,
                     std::string name,
                     std::string description,
                     std::string startDate,
                     std::string endDate,
                     std::vector<int> issueIds)
    : id_{id}, description_{std::move(description)} {
  if (id < -1) {
    throw std::invalid_argument("Milestone id must be >= -1");
  }
  setName(std::move(name));
  setStartDate(std::move(startDate));
  setEndDate(std::move(endDate));
  replaceIssues(std::move(issueIds));
}

void Milestone::validateRequiredField(const std::string& value,
                                      const char* field) {
  if (value.empty()) {
    throw std::invalid_argument(std::string(field) + " cannot be empty");
  }
}

void Milestone::validateIssueId(int issueId) {
  if (issueId <= 0) {
    throw std::invalid_argument("Issue id must be positive");
  }
}

void Milestone::setIdForPersistence(int id) {
  if (hasPersistentId()) {
    throw std::logic_error("Milestone already has a persistent id");
  }
  if (id < 0) {
    throw std::invalid_argument("Persistent id must be >= 0");
  }
  id_ = id;
}

void Milestone::setName(std::string name) {
  validateRequiredField(name, "name");
  name_ = std::move(name);
}

void Milestone::setDescription(std::string description) noexcept {
  description_ = std::move(description);
}

void Milestone::setStartDate(std::string startDate) {
  validateRequiredField(startDate, "start date");
  start_date_ = std::move(startDate);
}

void Milestone::setEndDate(std::string endDate) {
  validateRequiredField(endDate, "end date");
  end_date_ = std::move(endDate);
}

void Milestone::setSchedule(std::string startDate, std::string endDate) {
  setStartDate(std::move(startDate));
  setEndDate(std::move(endDate));
}

void Milestone::replaceIssues(std::vector<int> issueIds) {
  for (int id : issueIds) {
    validateIssueId(id);
  }
  ensureUnique(&issueIds);
  issue_ids_ = std::move(issueIds);
}

void Milestone::addIssue(int issueId) {
  validateIssueId(issueId);
  if (!hasIssue(issueId)) {
    issue_ids_.push_back(issueId);
  }
}

void Milestone::removeIssue(int issueId) {
  auto it = std::remove(issue_ids_.begin(), issue_ids_.end(), issueId);
  if (it != issue_ids_.end()) {
    issue_ids_.erase(it, issue_ids_.end());
  }
}

bool Milestone::hasIssue(int issueId) const noexcept {
  return std::find(issue_ids_.begin(), issue_ids_.end(), issueId) !=
         issue_ids_.end();
}
