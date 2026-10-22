cmake_minimum_required(VERSION 3.16)
project(frame-power-notifier LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

# openvr.h is not vendored here; see README ("Getting the OpenVR SDK").
set(OPENVR_DIR ${CMAKE_SOURCE_DIR}/third_party/openvr)
if(NOT EXISTS ${OPENVR_DIR}/openvr.h)
  message(FATAL_ERROR "Missing ${OPENVR_DIR}/openvr.h - see README")
endif()

# SteamVR ships libopenvr_api.so; point OPENVR_LIB at it if it is not found.
find_library(OPENVR_LIB openvr_api
  HINTS $ENV{HOME}/.local/share/Steam/steamapps/common/SteamVR/bin/linuxarm64
        $ENV{HOME}/.local/share/Steam/steamapps/common/SteamVR/bin/linux64
        ${OPENVR_DIR}/lib)
if(NOT OPENVR_LIB)
  message(FATAL_ERROR "libopenvr_api.so not found; pass -DOPENVR_LIB=/path/to/libopenvr_api.so")
endif()

add_executable(frame-power-notifier
  src/main.cpp
  src/power.cpp
  src/overlay.cpp)
target_include_directories(frame-power-notifier PRIVATE ${OPENVR_DIR} ${CAIRO_INCLUDE_DIRS})
target_link_libraries(frame-power-notifier PRIVATE ${OPENVR_LIB} ${CAIRO_LIBRARIES})
target_compile_options(frame-power-notifier PRIVATE -Wall -Wextra)

install(TARGETS frame-power-notifier RUNTIME DESTINATION bin)
