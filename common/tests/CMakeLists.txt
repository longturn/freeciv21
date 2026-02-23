set(CMAKE_AUTOMOC ON)

add_executable(test_dio dio.cpp)
target_link_libraries(test_dio PRIVATE common Qt6::Test)
add_test(NAME test_dio COMMAND test_dio)
