set(CMAKE_AUTOMOC ON)

enable_testing(true)

# Turn the list of rulesets into something C++ will understand
list(JOIN RULESET_LIST "\", \"" ruleset_list_cpp)
if (NOT ${ruleset_list_cpp} STREQUAL "")
  set(ruleset_list_cpp "\"${ruleset_list_cpp}\"")
endif()

# Create the test file. This is ugly because we need to replace variables
# and use generator expressions.
configure_file(rulesets.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/rulesets.cpp.in)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/rulesets.cpp
              INPUT ${CMAKE_CURRENT_BINARY_DIR}/rulesets.cpp.in)

add_executable(test_rulesets ${CMAKE_CURRENT_BINARY_DIR}/rulesets.cpp)
target_link_libraries(test_rulesets PRIVATE Qt5::Test)
add_test(NAME test_rulesets
         COMMAND test_rulesets
         WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
