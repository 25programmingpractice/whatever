cmake_minimum_required(VERSION 3.16)

project(whatever VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(WIN32)
    enable_language(RC)
endif()

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia)

set(PROJECT_SOURCES
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    playlistmodel.cpp
    playlistmodel.h
)

if(WIN32)
    set(RC_SCRIPT "assets/whatever.rc" CACHE STRING "")
endif()

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(whatever
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${RC_SCRIPT}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET whatever APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(whatever SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(whatever
            ${PROJECT_SOURCES}
        )
    endif()
endif()

qt_add_resources(whatever icons
    PREFIX "/"
    FILES
        assets/material-symbols-music-cast-rounded.png
        assets/material-symbols--format-list-bulleted-rounded.png
        assets/material-symbols--lyrics-outline-rounded.png
        assets/material-symbols--pause-rounded.png
        assets/material-symbols--play-arrow-rounded.png
        assets/material-symbols--playlist-play-rounded.png
        assets/material-symbols--repeat-one-rounded.png
        assets/material-symbols--shuffle-rounded.png
        assets/material-symbols--skip-next-rounded.png
        assets/material-symbols--skip-previous-rounded.png
        assets/material-symbols--volume-off-rounded.png
        assets/material-symbols--volume-up-rounded.png
        assets/material-symbols--delete-forever-rounded.png
)

target_link_libraries(whatever PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.whatever)
endif()
set_target_properties(whatever PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS whatever
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(whatever)
endif()
