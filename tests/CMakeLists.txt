include(FetchContent)
set(CATCH_VERSION v3.5.4)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG ${CATCH_VERSION})
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

function(argsbarg_add_test exe)
  add_executable(${exe} ${exe}.cpp)
  target_link_libraries(${exe} PRIVATE argsbarg Catch2::Catch2WithMain)
  catch_discover_tests(${exe})
endfunction()

argsbarg_add_test(test_validate)
argsbarg_add_test(test_parse)
argsbarg_add_test(test_help)
argsbarg_add_test(test_completion)
argsbarg_add_test(test_style)
argsbarg_add_test(test_context)

add_executable(test_smoke test_smoke.cpp)
target_link_libraries(test_smoke PRIVATE argsbarg Catch2::Catch2WithMain)
target_compile_definitions(test_smoke PRIVATE
  ARGSBARG_TEST_MINIMAL_EXE="$<TARGET_FILE:minimal_example>")
add_dependencies(test_smoke minimal_example nested_example)
target_compile_definitions(test_smoke PRIVATE
  ARGSBARG_TEST_NESTED_EXE="$<TARGET_FILE:nested_example>")
catch_discover_tests(test_smoke)
