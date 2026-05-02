find_package(Qt6 REQUIRED COMPONENTS Core Test)

add_executable(test_wavefile test_wavefile.cpp)

target_include_directories(test_wavefile PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test_wavefile PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME wavefile COMMAND test_wavefile)
