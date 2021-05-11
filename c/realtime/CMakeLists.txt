cmake_minimum_required(VERSION 2.8.12)
project(realtime)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()


add_executable(realtime src/main.c src/protocol.c src/filter_auto_lms.c)
target_link_libraries(realtime ${CONAN_LIBS})
