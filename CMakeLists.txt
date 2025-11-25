cmake_minimum_required(VERSION 3.10)
project(BrayaBrowser VERSION 1.0.9 LANGUAGES CXX)                                                                                                  
                                                                                                                                                   
set(CMAKE_CXX_STANDARD 17)                                                                                                                         
set(CMAKE_CXX_STANDARD_REQUIRED ON)                                                                                                                
                                                                                                                                                   
# Find required packages                                                                                                                           
# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)
pkg_check_modules(WEBKIT REQUIRED webkitgtk-6.0)
pkg_check_modules(OPENSSL REQUIRED openssl)
pkg_check_modules(JSON_GLIB REQUIRED json-glib-1.0)
find_library(LIBSODIUM_LIBRARY sodium)
if(NOT LIBSODIUM_LIBRARY)
    message(FATAL_ERROR "libsodium not found. Install libsodium-devel.")
endif()
                                                                                                                                                   
# Include directories                                                                                                                              
include_directories(
    ${GTK4_INCLUDE_DIRS}
    ${WEBKIT_INCLUDE_DIRS}
    ${OPENSSL_INCLUDE_DIRS}
    ${JSON_GLIB_INCLUDE_DIRS}
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
    src/BrayaPasswordManager_Extensions.cpp
    src/extensions/BrayaExtensionManager.cpp
    src/extensions/BrayaWebExtension.cpp
    src/extensions/BackgroundPageRunner.cpp
    src/extensions/BrayaExtensionAPI.cpp
    src/extensions/ExtensionInstaller.cpp
    src/adblocker/BrayaAdBlocker.cpp
    src/BrayaBookmarkIO.cpp
)

# Web extension source files (separate shared library)
set(WEB_EXTENSION_SOURCES
    src/extensions/web-extension/BrayaWebExtension.cpp
)

# Main executable
add_executable(braya-browser ${SOURCES})

# Web extension shared library
add_library(braya-web-extension SHARED ${WEB_EXTENSION_SOURCES})                                                                                                           
                                                                                                                                                   
# Link libraries for main browser
target_link_libraries(braya-browser
    ${GTK4_LIBRARIES}
    ${WEBKIT_LIBRARIES}
    ${OPENSSL_LIBRARIES}
    ${JSON_GLIB_LIBRARIES}
    ${LIBSODIUM_LIBRARY}
)

# Link libraries for web extension
target_link_libraries(braya-web-extension
    ${WEBKIT_LIBRARIES}
    ${GTK4_LIBRARIES}
)                                                                                                                                                  
                                                                                                                                                   
# Install targets
install(TARGETS braya-browser DESTINATION bin)
install(TARGETS braya-web-extension DESTINATION lib/braya-browser/web-extensions)                                                                                                     
                                                                                                                                                   
# Install resources                                                                                                                                
install(FILES resources/style.css DESTINATION share/braya-browser/resources)                                                                       
install(FILES resources/theme-dark.css DESTINATION share/braya-browser/resources)                                                                  
install(FILES resources/theme-light.css DESTINATION share/braya-browser/resources)                                                                 
install(FILES resources/theme-industrial.css DESTINATION share/braya-browser/resources)                                                            
install(FILES resources/home.html DESTINATION share/braya-browser/resources)
install(FILES resources/password-detect.js DESTINATION share/braya-browser/resources)
install(FILES resources/eff-wordlist-short.txt DESTINATION share/braya-browser/resources)                                                                       
                                                                                                                                                   
# Install icons at multiple sizes (both legacy and application ID names)
set(BRAYA_ICON_SIZES 16 32 48 64 128 256 512)
foreach(size IN LISTS BRAYA_ICON_SIZES)
    install(
        FILES resources/icons/braya-browser-${size}.png
        DESTINATION share/icons/hicolor/${size}x${size}/apps
        RENAME braya-browser.png
    )
    install(
        FILES resources/icons/dev.braya.BrayaBrowser-${size}.png
        DESTINATION share/icons/hicolor/${size}x${size}/apps
        RENAME dev.braya.BrayaBrowser.png
    )
endforeach()
                                                                                                                                                   
# Install desktop file                                                                                                                             
install(FILES braya-browser.desktop DESTINATION share/applications)
