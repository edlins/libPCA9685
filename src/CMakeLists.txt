cmake_minimum_required(VERSION 3.0)
project (libPCA9685)

set (libPCA9685_VERSION_MAJOR 0)
set (libPCA9685_VERSION_MINOR 7)
configure_file (
  "${PROJECT_SOURCE_DIR}/libPCA9685Config.h.in"
  "${PROJECT_BINARY_DIR}/libPCA9685Config.h"
)
include_directories ("${PROJECT_BINARY_DIR}")
add_library(PCA9685 SHARED PCA9685.c)

install (TARGETS PCA9685 DESTINATION lib)
install (FILES PCA9685.h DESTINATION include)
