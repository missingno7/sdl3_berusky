# Berusky for Android (skeleton)

Status: **written but not built or run** - the machine this was prepared on has
no Android SDK / NDK, so nothing here has been compiled. Treat it as a
starting point that follows the SDL3 `android-project` template.

The game is the same code as the desktop build:

* `../CMakeLists.txt` builds `libmain.so` (`src/main.cpp` + `berusky_core`) and
  SDL3 / SDL3_image as shared libraries (downloaded by CMake). The editor is
  not built (`-DBERUSKY_ENABLE_EDITOR=OFF`).
* `app/build.gradle` copies `../data` (Graphics, GameData, Levels, berusky.ini)
  into the APK assets. The game reads assets through `SDL_IOFromFile()`; the
  asset root is empty on Android (`src/platform.cpp`).
* Writable data (config, profiles, user levels) is in the app storage
  (`SDL_GetPrefPath`).
* Touch: on-screen controls while playing (`src/touch_controls.cpp`, on by default
  on Android), menus by direct touch (fingers -> logical game coordinates).
  The Android back button is the Escape key.
* Scaling on Android defaults to `scale_mode = fit` (the picture fills the screen
  keeping its aspect ratio).

## Build

Requirements: Android Studio (or SDK + NDK 28.2 + CMake), JDK 17+, network for
the first CMake configure (SDL is downloaded).

```
cd android
./gradlew assembleDebug
```

## Known open points

* Not tested on a device or emulator at all.
* Text input (profile names) needs `SDL_StartTextInput` / `SDL_EVENT_TEXT_INPUT`.
* No app icon of its own (SDL's default), no audio (the game has none).
* The double-size question at first start is asked by touch; on small screens
  the high resolution mode is probably the better default.
