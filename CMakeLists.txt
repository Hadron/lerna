cmake_minimum_required(VERSION 2.8)
project(lerna)

option(build_test "Build test demo?" ON)

include_directories(inc/)
set(lerna_SRC inc/lerna.h src/lerna.c)
add_library(lerna_static STATIC ${lerna_SRC})
add_library(lerna SHARED ${lerna_SRC})

if(build_test)
  add_executable(lernatest inc/lerna.h test/main.cpp)
  target_link_libraries(lernatest lerna hidapi-libusb GL GLU GLEW glfw tinycthread)
endif(build_test)
