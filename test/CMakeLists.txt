add_executable(test_trecpp test_trecpp.cpp)
target_link_libraries(test_trecpp
    trecpp
    Catch2
)
add_test(test_trecpp test_trecpp)
