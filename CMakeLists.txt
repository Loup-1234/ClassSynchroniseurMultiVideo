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

add_executable(ClassSynchroniseurMultiVideo
        include/ClassSynchroniseurMultiVideo/SynchroniseurMultiVideo.h
        src/main.cpp
        src/SynchroniseurMultiVideo.cpp
)

target_link_libraries(ClassSynchroniseurMultiVideo PRIVATE PkgConfig::FFMPEG)

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
    set_target_properties(doxygen_generate sphinx_generate PROPERTIES FOLDER "Documentation")
endif()