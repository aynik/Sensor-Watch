/*
* MIT License
 *
 * Copyright (c) 2024 Maybe Pablo
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "movement.h"

#ifndef MAGIC_8_BALL_FACE_H
#define MAGIC_8_BALL_FACE_H

/*
 * MAGIC 8 BALL FACE
 *
 * Displays a magic 8 ball that responds to the user's questions.
 *
 * Hold LIGHT or ALARM to shake the ball for the amount of time the button is held.
 * Release the button to see the answer.
 */

typedef struct {
    uint8_t decision : 5;
    uint8_t animation_type : 3;
    uint8_t animation_frame;
} magic_8_ball_state_t;

void magic_8_ball_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void magic_8_ball_face_activate(movement_settings_t *settings, void *context);
bool magic_8_ball_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void magic_8_ball_face_resign(movement_settings_t *settings, void *context);

#define magic_8_ball_face ((const watch_face_t){ \
magic_8_ball_face_setup, \
magic_8_ball_face_activate, \
magic_8_ball_face_loop, \
magic_8_ball_face_resign, \
NULL, \
})

#endif //MAGIC_8_BALL_FACE_H
