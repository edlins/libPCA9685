cmake_minimum_required(VERSION 3.0)

project(audio)
add_executable(6735l1 6735l1.c)
target_link_libraries(6735l1 asound)
add_executable(6735l2 6735l2.c)
target_link_libraries(6735l2 asound)
add_executable(6735l3 6735l3.c)
target_link_libraries(6735l3 asound)
add_executable(6735l4 6735l4.c)
target_link_libraries(6735l4 asound)
add_executable(vupeak vupeak.c)
target_link_libraries(vupeak asound PCA9685 fftw3 m)
add_executable(hello_fft.bin hello_fft.c mailbox.c gpu_fft.c gpu_fft_base.c gpu_fft_twiddles.c gpu_fft_shaders.c gpu_fft.h mailbox.h)
target_link_libraries(hello_fft.bin rt m dl)

add_custom_target(audio)
add_dependencies(audio 6735l1 6735l2 6735l3 6735l4 vupeak)

install(TARGETS vupeak DESTINATION bin)
install(FILES vupeak.service DESTINATION /etc/systemd/system)
install(FILES asound.conf DESTINATION /etc)

install(CODE "message(\"execute_process(COMMAND /bin/systemctl daemon-reload)\")")
install(CODE "execute_process(COMMAND /bin/systemctl daemon-reload)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl enable vupeak.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl enable vupeak.service)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl restart vupeak.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl restart vupeak.service)")

install(CODE "message(\"execute_process(COMMAND /bin/systemctl status vupeak.service)\")")
install(CODE "execute_process(COMMAND /bin/systemctl status vupeak.service)")

install(CODE "message(\"DONE!\")")
