set(CMAKE_AUTOMOC ON)

# Create the test file. This is ugly because we need to replace variables
# and use generator expressions.
configure_file(cli.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/cli.cpp.in)
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cli.cpp
              INPUT ${CMAKE_CURRENT_BINARY_DIR}/cli.cpp.in)

add_executable(test_server_cli ${CMAKE_CURRENT_BINARY_DIR}/cli.cpp)
target_link_libraries(test_server_cli PRIVATE Qt5::Test)
add_test(NAME test_server_cli
         COMMAND test_server_cli
         WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
