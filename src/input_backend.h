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
 * Platform input backend (SDL3).
 *
 * The only place that reads SDL events. It translates them to
 *
 *   - neutral keys              -> input::key_input()     (input.h, K_xxx)
 *   - pointer in game coords    -> input::mouse_input()   (mouse, touch)
 *
 * and handles window events (repaint after expose / resize).
 */

#ifndef __INPUT_BACKEND_H__
#define __INPUT_BACKEND_H__

class input;

// Process pending platform events (or wait for at least one when wait is set).
// Returns true when the application should quit.
bool input_backend_poll(class input *p_input, bool wait);

#endif // __INPUT_BACKEND_H__
