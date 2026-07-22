cmake_minimum_required(VERSION 3.20)

project(matching_engine LANGUAGES CXX)

add_executable(matching_engine
    app/main.cpp
    src/order_book.cpp
    src/matching_engine.cpp
)

target_include_directories(matching_engine PRIVATE include)
target_compile_features(matching_engine PRIVATE cxx_std_20)

enable_testing()

add_executable(matching_engine_tests
    tests/test_engine.cpp
    src/order_book.cpp
    src/matching_engine.cpp
)

target_include_directories(matching_engine_tests PRIVATE include)
target_compile_features(matching_engine_tests PRIVATE cxx_std_20)

add_test(NAME matching_engine_tests COMMAND matching_engine_tests)
