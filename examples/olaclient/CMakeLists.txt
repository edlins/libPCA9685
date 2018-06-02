# olaclient depends on libola-dev, libPCA9685, and systemd

cmake_minimum_required(VERSION 3.0)
project (olaclient)

add_executable(olaclient olaclient.cpp)

target_link_libraries(olaclient PCA9685)
target_link_libraries(olaclient ola)
target_link_libraries(olaclient olacommon)

install(TARGETS  olaclient    DESTINATION bin)
install(PROGRAMS olac_wrapper DESTINATION bin)
install(FILES    olac.service DESTINATION /etc/systemd/system/multi-user.target.wants)

install(CODE "execute_process(COMMAND /bin/systemctl enable olac.service)")
install(CODE "execute_process(COMMAND /bin/systemctl daemon-reload)")
install(CODE "execute_process(COMMAND /bin/systemctl restart olac.service)")
