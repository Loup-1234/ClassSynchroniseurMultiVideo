cmake_minimum_required(VERSION 4.1)
project(ClassSynchroniseurMultiVideo)

set(CMAKE_CXX_STANDARD 23)

set(SOURCE_FILES
        src/main.cpp
        src/SynchroniseurMultiVideo.cpp
)

set(HEADER_FILES
        include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h
)

add_executable(ClassSynchroniseurMultiVideo ${SOURCE_FILES} ${HEADER_FILES})

if (WIN32) # WINDOWS

    message(STATUS "Configuration pour Windows...")

    set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/include/ffmpeg")

    target_include_directories(ClassSynchroniseurMultiVideo PRIVATE ${FFMPEG_DIR}/include)

    target_link_directories(ClassSynchroniseurMultiVideo PRIVATE ${FFMPEG_DIR}/lib)

    target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE
            avcodec
            avformat
            avutil
            swscale
            swresample
    )

else () # LINUX

    message(STATUS "Configuration pour Linux...")

    find_package(PkgConfig REQUIRED)

    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
            libswscale
            libswresample
    )

    target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE PkgConfig::FFMPEG)

endif ()

target_include_directories(ClassSynchroniseurMultiVideo PRIVATE include)

# Doxygen

find_package(Doxygen)

if (DOXYGEN_FOUND)
    # 1. Commande pour exécuter Doxygen
    add_custom_target(doxygen_generate
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
            COMMENT "Génération du XML Doxygen..."
    )

    # 2. Commande pour exécuter Sphinx
    add_custom_target(sphinx_generate
            COMMAND sphinx-build -b html source build/html
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
            COMMENT "Génération du HTML Sphinx..."
            DEPENDS doxygen_generate # Sphinx attend que Doxygen ait fini
    )

    # Grouper dans "Documentation"
    set_target_properties(doxygen_generate sphinx_generate PROPERTIES FOLDER "Documentation")

endif ()