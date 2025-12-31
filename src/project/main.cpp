#include "IssueTrackerController.hpp"
#include "IssueRepository.hpp"

int main() {
    auto repo = createIssueRepository();
    IssueTrackerController controller(repo);
    return 0;
}
