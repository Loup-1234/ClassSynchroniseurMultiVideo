cmake_minimum_required(VERSION 4.1)
project(ClassSynchroniseurMultiVideo)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswscale
        libswresample
)

add_executable(ClassSynchroniseurMultiVideo main.cpp SynchroniseurMultiVideo.cpp SynchroniseurMultiVideo.h)

target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE PkgConfig::FFMPEG)