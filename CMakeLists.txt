cmake_minimum_required(VERSION 3.21)

# Single source of truth for the library semver (see also version() in argsbarg.hpp).
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/include/argsbarg/argsbarg.hpp" _argsbarg_hdr)
string(REGEX MATCH "return \"([0-9]+\\.[0-9]+\\.[0-9]+)\"" _argsbarg_match "${_argsbarg_hdr}")
set(_argsbarg_parse_ver "${CMAKE_MATCH_1}")
if(NOT _argsbarg_parse_ver)
  message(FATAL_ERROR "Could not parse semver from include/argsbarg/argsbarg.hpp (expected return \"X.Y.Z\" in version()).")
endif()

project(argsbarg
  VERSION ${_argsbarg_parse_ver}
  DESCRIPTION "C++23 header-only CLI toolkit"
  HOMEPAGE_URL "https://github.com/bdombro/cpp-argsbarg"
  LANGUAGES CXX
)

unset(_argsbarg_hdr)
unset(_argsbarg_match)
unset(_argsbarg_parse_ver)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# When embedded via FetchContent/add_subdirectory, default off to avoid extra targets
# and Catch2 fetch unless the consumer opts in.
if(PROJECT_IS_TOP_LEVEL)
  set(_argsbarg_build_examples_default ON)
  set(_argsbarg_build_tests_default ON)
else()
  set(_argsbarg_build_examples_default OFF)
  set(_argsbarg_build_tests_default OFF)
endif()
option(ARGSBARG_BUILD_EXAMPLES "Build example programs" ${_argsbarg_build_examples_default})
option(ARGSBARG_BUILD_TESTS "Build unit tests (Catch2 via FetchContent)" ${_argsbarg_build_tests_default})

option(ARGSBARG_ENABLE_COVERAGE "Instrument library and dependents for coverage" OFF)
if(ARGSBARG_ENABLE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "AppleClang|Clang")
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
  elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(--coverage)
    add_link_options(--coverage)
  endif()
endif()

add_library(argsbarg INTERFACE)
# Alias so both FetchContent and find_package consumers use the same target name.
add_library(argsbarg::argsbarg ALIAS argsbarg)

target_include_directories(argsbarg INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Ensure find_package / target_link_libraries consumers compile as C++23.
target_compile_features(argsbarg INTERFACE cxx_std_23)

if(ARGSBARG_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ARGSBARG_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ---------------------------------------------------------------------------
# Install layout + CMake package config
# ---------------------------------------------------------------------------
include(CMakePackageConfigHelpers)

set(ARGSBARG_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/argsbarg")

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/argsbarg"
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  COMPONENT argsbarg_Development
)

install(TARGETS argsbarg
  EXPORT argsbargTargets
  COMPONENT argsbarg_Development
)

install(EXPORT argsbargTargets
  FILE argsbargTargets.cmake
  NAMESPACE argsbarg::
  DESTINATION "${ARGSBARG_INSTALL_CMAKEDIR}"
  COMPONENT argsbarg_Development
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/argsbargConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/argsbargConfig.cmake"
  INSTALL_DESTINATION "${ARGSBARG_INSTALL_CMAKEDIR}"
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/argsbargConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/argsbargConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/argsbargConfigVersion.cmake"
  DESTINATION "${ARGSBARG_INSTALL_CMAKEDIR}"
  COMPONENT argsbarg_Development
)
