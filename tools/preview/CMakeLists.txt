# Desktop preview of the dashboard (macOS / Linux). Requires CMake >= 3.16 and SDL2.
#
#   cmake -S tools/preview -B build/preview -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/preview -j
#   ./build/preview/preview --weather tools/preview/fixtures/paris.json --out paris.png
cmake_minimum_required(VERSION 3.16)
project(weather_dashboard_preview C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(POLICY CMP0169)
  cmake_policy(SET CMP0169 OLD)  # allow FetchContent_Populate for sources without their own CMake build
endif()

# Same library versions as platformio.ini
include(FetchContent)
FetchContent_Declare(lovyangfx GIT_REPOSITORY https://github.com/lovyan03/LovyanGFX.git GIT_TAG 1.2.36 GIT_SHALLOW TRUE)
FetchContent_Declare(arduinojson GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git GIT_TAG v7.4.2 GIT_SHALLOW TRUE)
foreach(dep lovyangfx arduinojson)
  FetchContent_GetProperties(${dep})
  if(NOT ${dep}_POPULATED)
    FetchContent_Populate(${dep})
  endif()
endforeach()

set(LGFX ${lovyangfx_SOURCE_DIR}/src)
file(GLOB LGFX_SOURCES CONFIGURE_DEPENDS
  ${LGFX}/lgfx/v1/*.cpp
  ${LGFX}/lgfx/v1/misc/*.cpp
  ${LGFX}/lgfx/v1/panel/Panel_Device.cpp
  ${LGFX}/lgfx/v1/panel/Panel_FrameBufferBase.cpp
  ${LGFX}/lgfx/v1/platforms/sdl/*.cpp
  ${LGFX}/lgfx/v1/lv_font/*.c
  ${LGFX}/lgfx/utility/*.c
  ${LGFX}/lgfx/Fonts/efont/*.c
  ${LGFX}/lgfx/Fonts/IPA/*.c
  ${LGFX}/lgfx/Fonts/lvgl/*.c)
add_library(lovyangfx_desktop STATIC ${LGFX_SOURCES})
target_include_directories(lovyangfx_desktop SYSTEM PUBLIC ${LGFX})
target_compile_definitions(lovyangfx_desktop PUBLIC LGFX_SDL)
target_compile_options(lovyangfx_desktop PRIVATE -w)  # third-party code: keep the build output clean

find_package(SDL2 REQUIRED)
target_link_libraries(lovyangfx_desktop PUBLIC SDL2::SDL2)

set(APP ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
add_executable(preview
  preview.cpp
  ${APP}/ui/dashboard.cpp
  ${APP}/ui/icons.cpp
  ${APP}/ui/fonts.cpp
  ${APP}/weather/weather_parse.cpp)
target_include_directories(preview SYSTEM PRIVATE ${arduinojson_SOURCE_DIR}/src)
target_compile_options(preview PRIVATE -Wall -Wextra)
target_link_libraries(preview PRIVATE lovyangfx_desktop)
