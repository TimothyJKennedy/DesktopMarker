cmake_minimum_required(VERSION 3.25)
project(DesktopMarker VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Widgets)

set(SOURCES
    src/main.cpp
    src/settingsmanager.cpp
    src/strokemanager.cpp
    src/drawingoverlay.cpp
    src/hovertab.cpp
    src/settingsdialog.cpp
)

set(HEADERS
    src/settingsmanager.h
    src/stroke.h
    src/strokemanager.h
    src/drawingoverlay.h
    src/hovertab.h
    src/settingsdialog.h
)

qt_add_executable(DesktopMarker ${SOURCES} ${HEADERS})

target_link_libraries(DesktopMarker PRIVATE Qt6::Widgets)

if(WIN32)
    set_target_properties(DesktopMarker PROPERTIES WIN32_EXECUTABLE TRUE)
    target_link_libraries(DesktopMarker PRIVATE user32)
endif()


