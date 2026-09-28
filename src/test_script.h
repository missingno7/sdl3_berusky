/*
 * Berusky (C) AnakreoN
 * Martin Stransky <stransky@anakreon.cz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * Scripted input + framebuffer screenshots for regression tests.
 *
 * Active only when the environment variable BERUSKY_TEST_SCRIPT names a
 * script file. It injects input through the same neutral-key / logical-pointer
 * paths as real devices (input::key_input(), input::mouse_input()) and saves
 * the software framebuffer, so a whole session can be replayed and its
 * pictures compared pixel by pixel (see tests/run_tests.py).
 *
 * Script commands (one per line, '#' starts a comment; times are game ticks):
 *
 *   wait <ticks>           do nothing for the given number of game ticks
 *   key <name> [shift] [ctrl]  press and release a key (held for 2 ticks)
 *   keydown <name>         press a key
 *   keyup <name>           release a key
 *   move <x> <y>           move the pointer (logical game coordinates)
 *   click <x> <y>          move the pointer and click the left button
 *   shot <file.bmp>        save the framebuffer (the file goes to BERUSKY_TEST_OUT
 *                          when it's set)
 *   windowshot <file.bmp>  save what is in the window (scaled, letterboxed)
 *   window <w> <h>         resize the window
 *   fullscreen <0|1>       switch fullscreen
 *   quit                   quit the game
 *
 * Key names: a-z, 0-9, up, down, left, right, tab, enter, esc, space, bksp, f1-f12
 */

#ifndef __TEST_SCRIPT_H__
#define __TEST_SCRIPT_H__

class input;

// True when a script is running (the game uses it to be deterministic)
bool test_script_active(void);

// Game ticks (input polls) since the script started
long test_script_ticks(void);

// Called once per game tick by the input backend. Returns true to quit.
bool test_script_poll(class input *p_input);

#endif // __TEST_SCRIPT_H__
