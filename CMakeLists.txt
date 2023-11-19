cmake_minimum_required(VERSION 3.0)
project(libPCA9685)

# set the lib version
set(libPCA9685_VERSION_MAJOR 0)
set(libPCA9685_VERSION_MINOR 8)

# save the lib version in config.h
configure_file(config.h.cmake ${PROJECT_BINARY_DIR}/config.h)

# include for config.h
include_directories(${PROJECT_BINARY_DIR})

# add flags
set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -D_BSD_SOURCE -D_DEFAULT_SOURCE -std=c11 -Wall -pedantic -Wextra")

# build the library
add_subdirectory(src)

# build the test app
add_subdirectory(test EXCLUDE_FROM_ALL)

# build the examples, but not by default
add_subdirectory(examples EXCLUDE_FROM_ALL)

# add tests
enable_testing()
add_test(run_test sh -xc "./test/PCA9685test -td 1 40 > PCA9685_actual_output" 2>&1)
add_test(diff_output sh -xc "diff ./test/PCA9685_expected_output ./PCA9685_actual_output" 2>&1)

