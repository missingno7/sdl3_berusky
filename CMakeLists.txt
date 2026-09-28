cmake_minimum_required(VERSION 3.16)

project(berusky VERSION 1.7.2 LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Berusky - SDL3 port
#
#   berusky_core   - the game: logic, animation, levels, menus, software renderer,
#                    video/input backends. Everything that is shared by all
#                    platforms (Windows, Linux, macOS, Android).
#   berusky        - desktop executable (main.cpp only)
#
# The editor is optional (BERUSKY_ENABLE_EDITOR). A mobile build only needs
# the game path.
#
# The legacy Autotools build (configure.in, Makefile.am) is not used by this
# build and is not maintained any more.
# ---------------------------------------------------------------------------

option(BERUSKY_ENABLE_EDITOR "Build the level editor" ON)
option(BERUSKY_ENABLE_ASSERTS "Keep assert() in release builds (the game relies on them for sanity checks)" ON)
option(BERUSKY_FETCH_SDL "Download and build SDL3 / SDL3_image when they are not installed" ON)
option(BERUSKY_STATIC_SDL "Link the downloaded SDL3 / SDL3_image statically" ON)

set(BERUSKY_SDL3_TAG "release-3.4.16" CACHE STRING "SDL3 version to download")
set(BERUSKY_SDL3_IMAGE_TAG "release-3.4.6" CACHE STRING "SDL3_image version to download")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
endif()

# The code is from the C++98 era (it has its own "byte" type and so on)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BeruskyDependencies)

# ---------------------------------------------------------------------------
# Game core
# ---------------------------------------------------------------------------
set(BERUSKY_CORE_SOURCES
  src/2d_graph.cpp
  src/animation.cpp
  src/berusky.cpp
  src/berusky_gui.cpp
  src/data_parser.cpp
  src/data_parser_tables.cpp
  src/events.cpp
  src/game_logic.cpp
  src/graphics.cpp
  src/gui.cpp
  src/ini.cpp
  src/input.cpp
  src/level.cpp
  src/level_active.cpp
  src/level_changer.cpp
  src/level_game.cpp
  src/profile.cpp
  src/utils.cpp

  # SDL3 platform layer
  src/platform.cpp
  src/video.cpp
  src/input_sdl.cpp
  src/touch_controls.cpp
  src/test_script.cpp
)

if(BERUSKY_ENABLE_EDITOR)
  list(APPEND BERUSKY_CORE_SOURCES src/editor.cpp)
endif()

add_library(berusky_core STATIC ${BERUSKY_CORE_SOURCES})
target_include_directories(berusky_core PUBLIC src)
target_link_libraries(berusky_core PUBLIC SDL3::SDL3 SDL3_image::SDL3_image)

target_compile_definitions(berusky_core PUBLIC BERUSKY_VERSION_STRING="${PROJECT_VERSION}")
if(BERUSKY_ENABLE_EDITOR)
  target_compile_definitions(berusky_core PUBLIC BERUSKY_ENABLE_EDITOR=1)
endif()

# Where to look for the game data when it isn't found next to the executable
target_compile_definitions(berusky_core PRIVATE
  BERUSKY_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(MSVC)
  target_compile_options(berusky_core PUBLIC /utf-8 /wd4996)
  target_compile_definitions(berusky_core PUBLIC _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
  target_compile_options(berusky_core PUBLIC -fno-strict-aliasing)
  target_compile_options(berusky_core PRIVATE -Wall -Wno-write-strings -Wno-unused-but-set-variable
                                              -Wno-unused-variable -Wno-parentheses)
endif()

if(BERUSKY_ENABLE_ASSERTS)
  # Release configurations define NDEBUG - the original code base is written
  # with asserts on, keep them.
  foreach(cfg RELEASE RELWITHDEBINFO MINSIZEREL)
    foreach(lang C CXX)
      string(REGEX REPLACE "[-/]D ?NDEBUG" "" CMAKE_${lang}_FLAGS_${cfg} "${CMAKE_${lang}_FLAGS_${cfg}}")
    endforeach()
  endforeach()
endif()

# ---------------------------------------------------------------------------
# Desktop executable
# ---------------------------------------------------------------------------
add_executable(berusky src/main.cpp)
target_link_libraries(berusky PRIVATE berusky_core)

# ---------------------------------------------------------------------------
# Install: the executable + the game data (Graphics, GameData, Levels, ...)
# ---------------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS berusky RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY data/ DESTINATION ${CMAKE_INSTALL_DATADIR}/berusky
        PATTERN "Makefile*" EXCLUDE
        PATTERN "Windows" EXCLUDE
        PATTERN "Man" EXCLUDE
        PATTERN "spec" EXCLUDE)
