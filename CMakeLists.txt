cmake_minimum_required(VERSION 4.1)
project(ClassSynchroniseurMultiVideo)

set(CMAKE_CXX_STANDARD 23)

find_package(raylib CONFIG REQUIRED)

# --- Configuration des Sources ---

set(SOURCE_FILES
        src/main.cpp
        src/SynchroniseurMultiVideo.cpp
)

set(HEADER_FILES
        include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h
)

add_executable(ClassSynchroniseurMultiVideo ${SOURCE_FILES} ${HEADER_FILES})

# --- Configuration des Bibliothèques ---

if(WIN32)
    # --- WINDOWS ---
    message(STATUS "Configuration pour Windows...")

    target_include_directories(ClassSynchroniseurMultiVideo PRIVATE ${CMAKE_SOURCE_DIR}/include/ffmpeg/include)
    target_link_directories(ClassSynchroniseurMultiVideo PRIVATE ${CMAKE_SOURCE_DIR}/include/ffmpeg/lib)

    target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE
            raylib
            avcodec
            avformat
            avutil
            swscale
            swresample
    )

else()
    # --- LINUX ---
    message(STATUS "Configuration pour Linux...")

    find_package(PkgConfig REQUIRED)
    find_package(glfw3 CONFIG REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(Threads REQUIRED)

    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
            libavcodec
            libavformat
            libavutil
            libswscale
            libswresample
    )

    target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE
            raylib
            PkgConfig::FFMPEG
            glfw
            OpenGL::GL
            Threads::Threads
            m
            dl
            rt
    )

    target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE PkgConfig::FFMPEG)
endif()

target_include_directories(ClassSynchroniseurMultiVideo PRIVATE include)

# --- Documentation Target ---
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

    # Créer un groupe "Documentation" dans CLion pour faire propre
    set_target_properties(doxygen_generate sphinx_generate PROPERTIES FOLDER "Doc")
endif()