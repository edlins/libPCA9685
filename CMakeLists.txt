cmake_minimum_required(VERSION 3.6)
project(PCA9685)


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
