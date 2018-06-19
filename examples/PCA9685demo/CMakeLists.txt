cmake_minimum_required(VERSION 3.0)

project (PCA9685demo)
add_executable(PCA9685demo PCA9685demo.c)
target_link_libraries(PCA9685demo PCA9685)
target_link_libraries(PCA9685demo ncurses)

install(TARGETS PCA9685demo DESTINATION bin)
