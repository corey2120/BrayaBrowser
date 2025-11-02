cmake_minimum_required(VERSION 3.10)
project(BrayaBrowser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)

# Include directories
include_directories(
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${GTK4_LIBRARY_DIRS}
    ${WEBKIT_LIBRARY_DIRS}
)

# Source files
set(SOURCES
    src/main.cpp
    src/BrayaWindow.cpp
    src/BrayaTab.cpp
    src/BrayaSettings.cpp
    src/BrayaHistory.cpp
    src/BrayaDownloads.cpp
    src/BrayaBookmarks.cpp
    src/TabGroup.cpp
    src/BrayaCustomization.cpp
    src/BrayaPasswordManager.cpp
)

# Executable
add_executable(braya-browser ${SOURCES})

# Link libraries
target_link_libraries(braya-browser
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
)

# Install targets
install(TARGETS braya-browser DESTINATION bin)

# Install resources
install(FILES resources/style.css DESTINATION share/braya-browser/resources)
install(FILES resources/theme-dark.css DESTINATION share/braya-browser/resources)
install(FILES resources/theme-light.css DESTINATION share/braya-browser/resources)
install(FILES resources/theme-industrial.css DESTINATION share/braya-browser/resources)
install(FILES resources/home.html DESTINATION share/braya-browser/resources)

# Install icon
install(FILES resources/icons/braya-browser.png DESTINATION share/icons/hicolor/256x256/apps)

# Install desktop file
install(FILES braya-browser.desktop DESTINATION share/applications)
