find_package(Qt6 REQUIRED COMPONENTS Core Gui Test)

add_executable(test_wavefile test_wavefile.cpp)

target_include_directories(test_wavefile PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test_wavefile PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME wavefile COMMAND test_wavefile)

add_executable(test_viewstate
    test_viewstate.cpp
    ${CMAKE_SOURCE_DIR}/src/viewstate.cpp
)

target_include_directories(test_viewstate PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test_viewstate PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME viewstate COMMAND test_viewstate)

add_executable(test_waveformhelper
    test_waveformhelper.cpp
    ${CMAKE_SOURCE_DIR}/src/waveformrenderer.cpp
    ${CMAKE_SOURCE_DIR}/src/viewstate.cpp
)

target_include_directories(test_waveformhelper PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test_waveformhelper PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Test
)

add_test(NAME waveformhelper COMMAND test_waveformhelper)
