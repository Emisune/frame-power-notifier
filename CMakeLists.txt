cmake_minimum_required(VERSION 3.16)
project(frame-power-notifier LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# OpenVR header and library. The defaults are the copies SteamVR ships on the
# Frame; CI points these at the OpenVR SDK instead.
set(OPENVR_DIR /opt/steamvr/tools/hellovr_vulkan_linux/src/openvr/headers
    CACHE PATH "Directory containing openvr.h")
set(OPENVR_LIB /opt/steamvr/bin/linuxarm64/libopenvr_api.so
    CACHE FILEPATH "libopenvr_api.so to link against")
if(NOT EXISTS ${OPENVR_DIR}/openvr.h)
  message(FATAL_ERROR "openvr.h not found in ${OPENVR_DIR}; pass -DOPENVR_DIR=...")
endif()
if(NOT EXISTS ${OPENVR_LIB})
  message(FATAL_ERROR "${OPENVR_LIB} not found; pass -DOPENVR_LIB=...")
endif()

add_executable(frame-power-notifier
  src/main.cpp
  src/power.cpp
  src/alert.cpp
  src/icon.cpp
  src/overlay.cpp)
target_include_directories(frame-power-notifier PRIVATE ${OPENVR_DIR} ${CAIRO_INCLUDE_DIRS})
target_link_libraries(frame-power-notifier PRIVATE ${OPENVR_LIB} ${CAIRO_LIBRARIES})
target_compile_options(frame-power-notifier PRIVATE -Wall -Wextra)
# Always resolve libopenvr_api.so from the Frame's SteamVR at runtime, whatever
# it was linked against, and avoid depending on the build machine's libstdc++.
set_target_properties(frame-power-notifier PROPERTIES
  BUILD_WITH_INSTALL_RPATH ON
  INSTALL_RPATH /opt/steamvr/bin/linuxarm64)
target_link_options(frame-power-notifier PRIVATE -static-libstdc++ -static-libgcc)

# Renders the README screenshots with the overlay's own drawing code (cairo only).
add_executable(render-icons tools/render_icons.cpp src/icon.cpp)
target_include_directories(render-icons PRIVATE src ${CAIRO_INCLUDE_DIRS})
target_link_libraries(render-icons PRIVATE ${CAIRO_LIBRARIES})

enable_testing()
add_executable(alert-test tests/alert_test.cpp src/alert.cpp)
target_include_directories(alert-test PRIVATE src)
add_test(NAME alert COMMAND alert-test)
