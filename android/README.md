# Berusky for Android

Status: **builds (arm64-v8a + x86_64) and runs on the Android 15 emulator**
(Pixel 7 profile: 2400x1080 landscape, 420 dpi). Not tried on a physical
device yet.

The game is the same code as the desktop build:

* Sound and music (`../docs/AUDIO.md`): SDL3 audio streams, libxmp-lite is
  built into `libmain.so` by the same CMake (downloaded like SDL; linked with
  `liblog`, which the debug libxmp build logs to). The debug APK starts on the
  emulator with the audio device open (AAudio) and loads the modules; what
  happens to the audio in the background is left to SDL (not checked).
* `../CMakeLists.txt` builds `libmain.so` (`src/main.cpp` + `berusky_core`) and
  SDL3 / SDL3_image as shared libraries (downloaded by CMake). The editor is
  not built (`-DBERUSKY_ENABLE_EDITOR=OFF`).
* `app/build.gradle` copies `../data` (Graphics, GameData, Levels, Sound,
  Music, berusky.ini) into the APK assets. The game reads assets through
  `SDL_IOFromFile()`; the asset root is empty on Android (`src/platform.cpp`).
* Writable data (config, profiles, user levels) is in the app storage
  (`SDL_GetPrefPath`).
* Touch (`src/touch_controls.h`, on by default on Android), while playing:
  * swipe = one step; swipe and keep the finger down = walk on until it's
    lifted (move the held finger to turn). An indicator shows `>` for a step
    and `>>` for walking. Swipes made while the bug still walks are queued.
  * tap a bug in the top panel = select it (unavailable ones do nothing).
  * RESET and MENU buttons in the top-right corner.

  Menus by direct touch (fingers -> logical game coordinates). The Android
  back button is the Escape key.
* The same renderer as on the desktop (`docs/RENDERER.md`): the scene is
  rendered at the screen's resolution, the 4:3 composition is centered on a
  wide screen, touch controls use the free space beside it.

## Build

Requirements: Android Studio (or SDK + NDK 28.2 + CMake), JDK 17+, network for
the first CMake configure (SDL is downloaded).

```
cd android
./gradlew assembleDebug
```

## Known open points

* Checked on the emulator: start-up from packaged assets, the renderer at the
  screen's resolution (GLES2, 1440x1080 viewport = scale 2.25, pixelart
  filter), landscape lock + immersive fullscreen, menus by touch, Home ->
  return (the scene is presented again). The swipe controls are tested with
  injected finger events (`tests/scripts/12_touch.txt`, `22_touch_hud.txt`).
* Text input (profile names) needs `SDL_StartTextInput` / `SDL_EVENT_TEXT_INPUT`.
* No app icon of its own (SDL's default).
