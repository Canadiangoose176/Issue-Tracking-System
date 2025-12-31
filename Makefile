################################################################################
# Executables
################################################################################

PROJECT = project
REST    = its # CHANGED: Renamed from rest_server to its
GTEST   = test_${PROJECT}

################################################################################
# Compiler + Flags
################################################################################

CXX = g++
CXXVERSION = -std=c++17
CXXFLAGS = ${CXXVERSION} -g
CXXWITHCOVERAGEFLAGS = ${CXXFLAGS} -fprofile-arcs -ftest-coverage

################################################################################
# SQLite
################################################################################

SQLITE_PREFIX = third_party/sqlite-build

################################################################################
# OATPP (FIXED DEPTH)
################################################################################

OATPP_INCLUDE_LIB = /usr/local/include/oatpp-1.3.0/oatpp
OATPP_SWAGGER_INCLUDE = /usr/local/include/oatpp-1.3.0/oatpp-swagger
OATPP_LIB_DIR = /usr/local/lib/oatpp-1.3.0

OATPP_INCLUDE = -I $(OATPP_INCLUDE_LIB) -I $(OATPP_SWAGGER_INCLUDE)

################################################################################
# Directories
################################################################################

SRC_DIR = src
MODEL_DIR = src/model
REPO_DIR = src/repository
VIEW_DIR = src/view
CONTROLLER_DIR = src/controller
SERVER_DIR = src/server
PROJECT_SRC_DIR = src/project
DTO_DIR = src/dto

GTEST_DIR = test
SRC_INCLUDE = include

################################################################################
# Include Paths
################################################################################

BASE_INCLUDE = \
    -I include \
    -I src \
    -I src/dto \
    -I third_party/sqlite-build/include

REST_INCLUDE = $(BASE_INCLUDE) $(OATPP_INCLUDE)

################################################################################
# Link Flags
################################################################################

BASE_LINKFLAGS = -lgtest -lgmock -pthread \
    -L $(SQLITE_PREFIX)/lib \
    -Wl,-rpath,$(abspath $(SQLITE_PREFIX)/lib) \
    -lsqlite3

OATPP_LINKFLAGS = \
    -L $(OATPP_LIB_DIR) \
    -Wl,-rpath,$(OATPP_LIB_DIR) \
    -loatpp-swagger -loatpp

################################################################################
# Tools
################################################################################

GCOV = gcov
LCOV = lcov
COVERAGE_RESULTS = results.coverage
COVERAGE_DIR = coverage
STATIC_ANALYSIS = cppcheck
STYLE_CHECK = cpplint
DOXY_DIR = docs/code

################################################################################
# Sources
################################################################################

CORE_SRCS = \
    $(wildcard ${SRC_DIR}/*.cpp) \
    $(wildcard ${MODEL_DIR}/*.cpp) \
    $(wildcard ${REPO_DIR}/*.cpp) \
    $(wildcard ${VIEW_DIR}/*.cpp) \
    $(wildcard ${CONTROLLER_DIR}/*.cpp)

REST_SRCS = ${CORE_SRCS} \
    $(wildcard ${SERVER_DIR}/*.cpp)

################################################################################
# Default target
################################################################################

.DEFAULT_GOAL := compileProject

################################################################################
# Clean
################################################################################

.PHONY: clean
clean:
	rm -rf *.gcov *.gcda *.gcno ${COVERAGE_RESULTS} ${COVERAGE_DIR}
	rm -rf docs/code/html
	rm -rf ${PROJECT} ${GTEST} ${REST} # Updated ${REST} variable
	rm -rf src/*.o src/model/*.o src/repository/*.o \
           src/view/*.o src/controller/*.o \
           src/server/*.o src/project/*.o
	rm -rf *~ \#* .\#* src/*~ test/*~ include/*~

################################################################################
# Object rule
################################################################################

%.o: %.cpp
	${CXX} ${CXXFLAGS} -c $< -o $@

################################################################################
# Build
################################################################################

# Tests: no oatpp
${GTEST}: clean
	${CXX} ${CXXFLAGS} -o ./${GTEST} ${BASE_INCLUDE} \
    ${GTEST_DIR}/*.cpp ${CORE_SRCS} ${BASE_LINKFLAGS}

# Main project: core only (no oatpp unless you really need it here)
compileProject: clean
	${CXX} ${CXXVERSION} -o ${PROJECT} ${BASE_INCLUDE} \
    ${CORE_SRCS} ${PROJECT_SRC_DIR}/*.cpp ${BASE_LINKFLAGS}

# REST server: this is where oatpp is required
its: clean # CHANGED: Target name changed from 'rest' to 'its'
	${CXX} ${CXXVERSION} -o ${REST} ${REST_INCLUDE} \
	${REST_SRCS} ${BASE_LINKFLAGS} ${OATPP_LINKFLAGS}

################################################################################
# Extra
################################################################################

memcheck: ${GTEST}
	valgrind --tool=memcheck --leak-check=yes \
    --error-exitcode=1 ./${GTEST}

coverage: clean
	${CXX} ${CXXWITHCOVERAGEFLAGS} -o ./${GTEST} ${BASE_INCLUDE} \
    ${GTEST_DIR}/*.cpp ${CORE_SRCS} ${BASE_LINKFLAGS}
	./${GTEST}
	${LCOV} --capture --gcov-tool ${GCOV} \
        --directory . --output-file ${COVERAGE_RESULTS} \
        --rc lcov_branch_coverage=1
	${LCOV} --extract ${COVERAGE_RESULTS} */*/*/${SRC_DIR}/* \
        -o ${COVERAGE_RESULTS}
	genhtml ${COVERAGE_RESULTS} --output-directory ${COVERAGE_DIR}

static:
	${STATIC_ANALYSIS} --verbose --enable=all ${SRC_DIR} \
    ${SRC_INCLUDE} --suppress=missingInclude \
    --error-exitcode=1

style:
	${STYLE_CHECK} ${SRC_DIR}/* ${GTEST_DIR}/* \
    ${SRC_INCLUDE}/* \
    ${MODEL_DIR}/* ${REPO_DIR}/* ${VIEW_DIR}/* \
    ${CONTROLLER_DIR}/* ${SERVER_DIR}/* \
    ${PROJECT_SRC_DIR}/*

docs:
	doxygen ${DOXY_DIR}/doxyfile

run:
	./${PROJECT}

run-its:
	./${REST}

################################################################################
# Required by CI
################################################################################

.PHONY: version
version:
	doxygen --version
	cppcheck --version
	cpplint --version
	gcc --version
	gcov --version
	lcov --version
	valgrind --version
