set(CMAKE_AUTOMOC ON)

add_executable(test_utility_paths test_paths.cpp)
target_link_libraries(test_utility_paths PRIVATE Qt5::Test utility)
add_test(NAME test_utility_paths COMMAND test_utility_paths)
