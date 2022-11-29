cmake_minimum_required(VERSION 3.0)

project (olaclient)

add_executable(olaclient olaclient.cpp)

target_link_libraries(olaclient PCA9685)
target_link_libraries(olaclient ola)
target_link_libraries(olaclient olacommon)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(olaclient Threads::Threads)

install(TARGETS olaclient DESTINATION bin)
install(FILES olaclient.service DESTINATION /etc/systemd/system)

install(CODE "message(\"execute_process(COMMAND /bin/systemctl daemon-reload)\")")
install(CODE "execute_process(COMMAND /bin/systemctl daemon-reload)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl enable olaclient.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl enable olaclient.service)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl restart olaclient.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl restart olaclient.service)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl status olaclient.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl status olaclient.service)")

install(CODE "message(\"DONE!\")")
