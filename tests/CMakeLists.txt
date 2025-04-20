add_executable(smoke smoke.cpp)
target_link_libraries(smoke PRIVATE iconv)
add_test(NAME smoke COMMAND smoke)
