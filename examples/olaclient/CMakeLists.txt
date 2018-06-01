cmake_minimum_required(VERSION 3.0)
project (olaclient)

add_executable(olaclient olaclient.cpp)

target_link_libraries(olaclient PCA9685)
target_link_libraries(olaclient ola)
target_link_libraries(olaclient olacommon)

install(TARGETS  olaclient    DESTINATION bin)
install(PROGRAMS olac_wrapper DESTINATION bin)
install(FILES    olac.service DESTINATION /lib/systemd/system)
install(FILES    olac.service DESTINATION /usr/lib/systemd/system)
install(FILES    olac.service DESTINATION /etc/systemd/system)
