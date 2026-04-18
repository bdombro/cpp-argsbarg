cmake_minimum_required(VERSION 3.20)
project(argsbarg LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ARGS_ENABLE_COVERAGE "Instrument library and dependents for coverage" OFF)
if(ARGS_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)
  endif()
endif()

add_library(argsbarg INTERFACE)
target_include_directories(argsbarg INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

add_subdirectory(examples)
enable_testing()
add_subdirectory(tests)
