cmake_minimum_required(VERSION 3.13)

project(shell-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.hpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard

add_executable(shell ${SOURCE_FILES})

if(APPLE)
  execute_process(
    COMMAND brew --prefix readline
    OUTPUT_VARIABLE BREW_READLINE_PREFIX
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )
  target_include_directories(shell PRIVATE ${BREW_READLINE_PREFIX}/include)
  target_link_libraries(shell PRIVATE ${BREW_READLINE_PREFIX}/lib/libreadline.dylib)
else()
  target_link_libraries(shell PRIVATE readline)
endif()