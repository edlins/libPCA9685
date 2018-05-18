cmake_minimum_required(VERSION 3.0)
project (olaclient)

set (libPCA9685_VERSION_MAJOR 0)
set (libPCA9685_VERSION_MINOR 7)
configure_file (
  "${PROJECT_SOURCE_DIR}/olaclientConfig.h.in"
  "${PROJECT_BINARY_DIR}/olaclientConfig.h"
)
include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}")

add_executable(olaclient EXCLUDE_FROM_ALL olaclient.cpp)

target_link_libraries(olaclient PCA9685)
target_link_libraries(olaclient ola)
target_link_libraries(olaclient olacommon)

install (TARGETS olaclient  DESTINATION bin)
install (PROGRAMS olac_wrapper DESTINATION bin)
install (PROGRAMS olac         DESTINATION /etc/init.d)
