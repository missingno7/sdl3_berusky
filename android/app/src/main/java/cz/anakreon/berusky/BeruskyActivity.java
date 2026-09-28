package cz.anakreon.berusky;

import org.libsdl.app.SDLActivity;

/**
 * The whole game is native code shared with the desktop builds
 * (berusky_core + main.cpp, built as libmain.so). SDLActivity loads
 * libSDL3.so and libmain.so and calls main().
 */
public class BeruskyActivity extends SDLActivity {

    @Override
    protected String[] getLibraries() {
        return new String[] {
            "SDL3",
            "main"
        };
    }
}
