**Last Updated:** 2025-11-05 by shah3720

## Tools Used

* **Compiler:** g++ (C++14)
* **Style Check:** cpplint
* **Static Check:** cppcheck
* **Unit Test:** GoogleTest (gtest)/ mocktest (gmock)
* **Documentation:** doxygen

---

## Assumptions

* One repo = one project.
* All **unit tests** are in `test/` (including the test `main.cpp`).
* All **headers** are in `include/`, named `*.hpp` (each class has a header).
* All **source** files are in `src/`, named `*.cpp`.

---

## Project Structure (this repo)

```
include/
  Comment.hpp
  Issue.hpp
  IssueRepository.hpp
  IssueTrackerController.hpp
  IssueTrackerView.hpp
  User.hpp
src/
  Comment.cpp
  Issue.cpp
  IssueRepository.cpp
  IssueTrackerController.cpp
  IssueTrackerView.cpp
  User.cpp
  project/
    main.cpp
test/
  CommentTest.cpp
  IssueTest.cpp
Makefile
.gitlab-ci.yml
README.md
```

---

## Build & Run (Local)
make clean
make its
./its
python3 -m http.server 8080
http://localhost:8080/docs/design/index.html

## Quality, Style, and Static Analysis

```bash
# Style check (cpplint)
make style

# Static analysis (cppcheck)
make static
```

---

## Code Coverage

```bash
# Run tests with coverage + generate HTML report (lcov/genhtml)
make coverage

# Open report:
#   ./coverage/index.html
```

*Notes:*

* New objects in models start with **id == 0** (not persisted).
* Repository assigns positive IDs once (persistence).
* Tests cover both **Issue** and **Comment** rules; aim for **>90%** total.

---

## Memory Check

```bash
# Run tests under valgrind (target may vary by Makefile)
make memcheck
```

---

## Documentation (Doxygen)

We use **Doxygen** comments (`@brief`, `@param`, `@return`, `@throws`) on **all
public methods/ctors**.

```bash
# Generate docs (if Doxyfile is present)
doxygen -g  # first time only, to create Doxyfile
doxygen

# Open:
#   ./html/index.html
```

---

## Architecture (MVC + DI)

* **Models:** `Issue`, `Comment`, `User`

  * Validate inputs; `Issue` stores **comment IDs** and full **Comment objects**.
* **Controller:** `IssueTrackerController`

  * Orchestrates create/assign/update; injected with a repository.
* **View:** `IssueTrackerView` (text UI)

  * Reads user input; calls controller.
* **Repository (DI):** `IssueRepository` (+ in-memory impl)

  * Assigns IDs, stores Issues/Comments/Users.

---

## Team

* **Hetansh** 
* **Fletcher** 
* **Colby** 
* **Miller** 

