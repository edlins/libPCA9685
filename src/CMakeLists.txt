cmake_minimum_required(VERSION 3.0)
project(libPCA9685)

# build the lib
add_library(PCA9685 SHARED PCA9685.c)

# install the lib
install(TARGETS PCA9685 DESTINATION lib)
install(FILES PCA9685.h DESTINATION include)
