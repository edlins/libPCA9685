project(examples)
# build the examples
add_subdirectory(olaclient)
add_subdirectory(PCA9685demo)
add_subdirectory(quickstart)
add_subdirectory(spectrum)

add_custom_target(examples)
add_dependencies(examples olaclient PCA9685demo quickstart spectrum)
