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
