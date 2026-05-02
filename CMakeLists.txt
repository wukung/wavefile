cmake_minimum_required(VERSION 3.16)

project(wavefile VERSION 1.0 LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS src)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_executable(wavefile
    src/main.cpp
    src/widget.cpp
    src/widget.ui
    src/pcmformatdialog.cpp
    include/widget.h
    include/WaveFile.h
    include/pcmformatdialog.h
)

target_include_directories(wavefile PRIVATE include)

target_link_libraries(wavefile PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
