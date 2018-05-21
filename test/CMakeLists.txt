cmake_minimum_required(VERSION 3.0)
project(PCA9685test)

# build the test app
add_executable(PCA9685test PCA9685test.c)

# link with the lib
target_link_libraries(PCA9685test PCA9685)
