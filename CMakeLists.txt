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
find_package(PkgConfig QUIET)

add_executable(wavefile
    src/main.cpp
    src/widget.cpp
    src/viewstate.cpp
    src/waveformrenderer.cpp
    src/spectrogramrenderer.cpp
    src/audiofileloader.cpp
    src/widget.ui
    src/pcmformatdialog.cpp
    include/widget.h
    include/viewstate.h
    include/waveformrenderer.h
    include/spectrogramrenderer.h
    include/audiofileloader.h
    include/WaveFile.h
    include/pcmformatdialog.h
)

target_include_directories(wavefile PRIVATE include)

target_link_libraries(wavefile PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

if(PkgConfig_FOUND)
    pkg_check_modules(FFTW3 QUIET IMPORTED_TARGET fftw3)
endif()

if(TARGET PkgConfig::FFTW3)
    target_link_libraries(wavefile PRIVATE PkgConfig::FFTW3)
    target_compile_definitions(wavefile PRIVATE WAVEFILE_USE_FFTW=1)
else()
    find_library(FFTW3_LIB fftw3)
    if(FFTW3_LIB)
        target_link_libraries(wavefile PRIVATE ${FFTW3_LIB})
        target_compile_definitions(wavefile PRIVATE WAVEFILE_USE_FFTW=1)
    else()
        message(WARNING "FFTW3 not found; spectrogram falls back to slower DFT implementation.")
    endif()
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
