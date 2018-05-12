cmake_minimum_required(VERSION 3.0)
project(PCA9685)

# set the lib version
set (libPCA9685_VERSION_MAJOR 0)
set (libPCA9685_VERSION_MINOR 7)

# add flags
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -Wall -pedantic -Wextra")

# add tests
enable_testing()
add_test(run_test sh -xc "./PCA9685test -td 1 40 > PCA9685_actual_output" 2>&1)
add_test(diff_output sh -xc "diff ../test/PCA9685_expected_output ./PCA9685_actual_output" 2>&1)


# build the library
add_subdirectory (src)
include_directories ("${PROJECT_SOURCE_DIR}/src")


# build the quickstart example
configure_file (
  "${PROJECT_SOURCE_DIR}/examples/quickstart/quickstartConfig.h.in"
  "${PROJECT_BINARY_DIR}/examples/quickstart/quickstartConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/examples/quickstart")
add_executable (quickstart examples/quickstart/quickstart.c)
target_link_libraries (quickstart PCA9685)


# build the PCA9685demo example
configure_file (
  "${PROJECT_SOURCE_DIR}/examples/PCA9685demo/PCA9685demoConfig.h.in"
  "${PROJECT_BINARY_DIR}/examples/PCA9685demo/PCA9685demoConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/examples/PCA9685demo")
add_executable (PCA9685demo examples/PCA9685demo/PCA9685demo.c)
target_link_libraries (PCA9685demo PCA9685)
target_link_libraries (PCA9685demo ncurses)


# build the PCA9685test app
configure_file (
  "${PROJECT_SOURCE_DIR}/test/PCA9685testConfig.h.in"
  "${PROJECT_BINARY_DIR}/test/PCA9685testConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/test")
add_executable (PCA9685test test/PCA9685test.c)
target_link_libraries (PCA9685test PCA9685)


# build the olaclient app
configure_file (
  "${PROJECT_SOURCE_DIR}/examples/olaclient/olaclientConfig.h.in"
  "${PROJECT_BINARY_DIR}/examples/olaclient/olaclientConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/examples/olaclient")
add_executable (olaclient examples/olaclient/olaclient.cpp)
target_link_libraries(olaclient PCA9685)
target_link_libraries(olaclient ola)
target_link_libraries(olaclient olacommon)
