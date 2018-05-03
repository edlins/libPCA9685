cmake_minimum_required(VERSION 3.0)
project(PCA9685)


# add flags
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -std=c11 -Wall -pedantic -Wextra")

enable_testing()
add_test(test1 sh -c "./PCA9685test -td 1 40 > ../test/PCA9685_actual_output")


# build the library
add_subdirectory (src)
include_directories ("${PROJECT_SOURCE_DIR}/src")


# build the quickstart example
set (quickstart_VERSION_MAJOR 0)
set (quickstart_VERSION_MINOR 1)
configure_file (
  "${PROJECT_SOURCE_DIR}/examples/quickstart/quickstartConfig.h.in"
  "${PROJECT_BINARY_DIR}/examples/quickstart/quickstartConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/examples/quickstart")
add_executable (quickstart examples/quickstart/quickstart.c)
target_link_libraries (quickstart PCA9685)


# build the PCA9685demo example
set (PCA9685demo_VERSION_MAJOR 0)
set (PCA9685demo_VERSION_MINOR 2)
configure_file (
  "${PROJECT_SOURCE_DIR}/examples/PCA9685demo/PCA9685demoConfig.h.in"
  "${PROJECT_BINARY_DIR}/examples/PCA9685demo/PCA9685demoConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/examples/PCA9685demo")
add_executable (PCA9685demo examples/PCA9685demo/PCA9685demo.c)
target_link_libraries (PCA9685demo PCA9685)
target_link_libraries (PCA9685demo ncurses)


# build the PCA9685test app
set (PCA9685test_VERSION_MAJOR 0)
set (PCA9685test_VERSION_MINOR 1)
configure_file (
  "${PROJECT_SOURCE_DIR}/test/PCA9685testConfig.h.in"
  "${PROJECT_BINARY_DIR}/test/PCA9685testConfig.h"
)
include_directories("${PROJECT_BINARY_DIR}/test")
add_executable (PCA9685test test/PCA9685test.c)
target_link_libraries (PCA9685test PCA9685)
