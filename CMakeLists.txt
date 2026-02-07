cmake_minimum_required(VERSION 3.16)
project(EShot VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(SOURCES
    src/main.cpp
    src/core/HotkeyManager.cpp
    src/capture/CaptureOverlay.cpp
    src/capture/PinnedWindow.cpp
    src/annotation/AnnotationEngine.cpp
    src/ui/AnnotationToolbar.cpp
    src/ui/SettingsDialog.cpp
    src/ui/AboutDialog.cpp
)

set(HEADERS
    src/core/HotkeyManager.h
    src/capture/CaptureOverlay.h
    src/capture/PinnedWindow.h
    src/annotation/AnnotationEngine.h
    src/ui/AnnotationToolbar.h
    src/ui/SettingsDialog.h
    src/ui/AboutDialog.h
)

set(RESOURCES
    resources/icons.qrc
)

qt_add_executable(${PROJECT_NAME} WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    User32
    Gdi32
    Shell32
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)