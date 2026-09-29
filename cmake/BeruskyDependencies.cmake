# SDL3 and SDL3_image.
#
# 1. An installed SDL3 / SDL3_image is used first (find_package).
# 2. Otherwise they are downloaded and built together with the game
#    (BERUSKY_FETCH_SDL), so a clean checkout builds without any preparation.
#
# Result: targets SDL3::SDL3 and SDL3_image::SDL3_image.

find_package(SDL3 CONFIG QUIET)
if(SDL3_FOUND)
  find_package(SDL3_image CONFIG QUIET)
endif()

if(NOT (SDL3_FOUND AND SDL3_image_FOUND))
  if(NOT BERUSKY_FETCH_SDL)
    message(FATAL_ERROR "SDL3 and SDL3_image were not found. Install them or set BERUSKY_FETCH_SDL=ON.")
  endif()

  message(STATUS "SDL3 / SDL3_image not installed - downloading ${BERUSKY_SDL3_TAG} / ${BERUSKY_SDL3_IMAGE_TAG}")

  include(FetchContent)

  if(BERUSKY_STATIC_SDL)
    set(BUILD_SHARED_LIBS OFF)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
  else()
    set(BUILD_SHARED_LIBS ON)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)
  endif()
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_INSTALL OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        ${BERUSKY_SDL3_TAG}
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(SDL3)

  # Game sprites are PNG. SDL3_image decodes them with its built-in
  # decoders, no external image libraries are needed.
  set(SDLIMAGE_VENDORED OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_INSTALL OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_SAMPLES OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_TESTS OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_DEPS_SHARED OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_AVIF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_JXL OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_TIF OFF CACHE BOOL "" FORCE)
  set(SDLIMAGE_WEBP OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(SDL3_image
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_image.git
    GIT_TAG        ${BERUSKY_SDL3_IMAGE_TAG}
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(SDL3_image)
endif()

# libxmp-lite (MIT) plays the music of the DOS original: FastTracker II
# modules, loaded as they are (docs/AUDIO.md). The lite library is only the
# MOD/S3M/XM/IT player, a few dozen C files.
#
# 1. An installed libxmp-lite or libxmp is used first (find_package).
# 2. Otherwise it's downloaded and built with the game (BERUSKY_FETCH_LIBXMP).
#
# Result: target berusky_xmp.

find_package(libxmp-lite CONFIG QUIET)
if(TARGET libxmp-lite::xmp_lite_shared)
  set(BERUSKY_XMP_TARGET libxmp-lite::xmp_lite_shared)
elseif(TARGET libxmp-lite::xmp_lite_static)
  set(BERUSKY_XMP_TARGET libxmp-lite::xmp_lite_static)
else()
  find_package(libxmp CONFIG QUIET)
  if(TARGET libxmp::xmp_shared)
    set(BERUSKY_XMP_TARGET libxmp::xmp_shared)
  elseif(TARGET libxmp::xmp_static)
    set(BERUSKY_XMP_TARGET libxmp::xmp_static)
  endif()
endif()

if(NOT BERUSKY_XMP_TARGET)
  if(NOT BERUSKY_FETCH_LIBXMP)
    message(FATAL_ERROR "libxmp-lite was not found. Install it or set BERUSKY_FETCH_LIBXMP=ON.")
  endif()

  message(STATUS "libxmp-lite not installed - downloading ${BERUSKY_LIBXMP_TAG}")

  include(FetchContent)

  set(BUILD_STATIC ON CACHE BOOL "" FORCE)
  set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
  set(BUILD_LITE ON CACHE BOOL "" FORCE)
  set(LIBXMP_DISABLE_DEPACKERS ON CACHE BOOL "" FORCE)
  set(LIBXMP_DISABLE_PROWIZARD ON CACHE BOOL "" FORCE)
  set(LIBXMP_DOCS OFF CACHE BOOL "" FORCE)
  # It goes into libmain.so on Android
  set(LIBXMP_PIC ON CACHE BOOL "" FORCE)

  FetchContent_Declare(libxmp
    GIT_REPOSITORY https://github.com/libxmp/libxmp.git
    GIT_TAG        ${BERUSKY_LIBXMP_TAG}
    GIT_SHALLOW    TRUE)
  FetchContent_MakeAvailable(libxmp)

  # Only the lite player is needed - don't build the full library
  if(TARGET xmp_static)
    set_target_properties(xmp_static PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()
  set(BERUSKY_XMP_TARGET libxmp-lite::xmp_lite_static)
endif()

add_library(berusky_xmp INTERFACE)
target_link_libraries(berusky_xmp INTERFACE ${BERUSKY_XMP_TARGET})
