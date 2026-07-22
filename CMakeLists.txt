cmake_minimum_required(VERSION 3.20)

project(matching_engine VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(matching_engine_core
    src/matching_engine.cpp
    src/order_book.cpp
)

target_include_directories(matching_engine_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(matching_engine_core PUBLIC cxx_std_20)
set_target_properties(matching_engine_core PROPERTIES CXX_EXTENSIONS OFF)

add_executable(matching_engine app/main.cpp)
target_link_libraries(matching_engine PRIVATE matching_engine_core)

if(BUILD_TESTING)
    add_executable(matching_engine_tests tests/test_engine.cpp)
    target_link_libraries(matching_engine_tests PRIVATE matching_engine_core)

    add_test(NAME matching_engine_tests COMMAND matching_engine_tests)
endif()
