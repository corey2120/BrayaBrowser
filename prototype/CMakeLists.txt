cmake_minimum_required(VERSION 3.10)
project(CEF-GTK4-Prototype CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

# CEF paths
set(CEF_INCLUDE_DIR "/usr/include/cef")
set(CEF_LIBRARY_DIR "/usr/lib64/cef")
set(CEF_LIBRARY "${CEF_LIBRARY_DIR}/libcef.so")

# Include directories
include_directories(
    ${CEF_INCLUDE_DIR}
    ${GTK4_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${CEF_LIBRARY_DIR}
    ${GTK4_LIBRARY_DIRS}
)

# Add executable
add_executable(cef-hello cef-hello.cpp)

# Compiler flags
target_compile_options(cef-hello PRIVATE
    ${GTK4_CFLAGS_OTHER}
    -DUSE_X11  # If using X11
)

# Link libraries
target_link_libraries(cef-hello
    ${CEF_LIBRARY}
    ${GTK4_LIBRARIES}
    pthread
    X11  # X11 support
)

# Set rpath so it can find libcef.so
set_target_properties(cef-hello PROPERTIES
    INSTALL_RPATH "${CEF_LIBRARY_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Install target
install(TARGETS cef-hello DESTINATION bin)
