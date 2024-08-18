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

// Emulator only: need time() to seed the random number generator.
#if __EMSCRIPTEN__
#include <time.h>
#endif

#include <stdlib.h>
#include <string.h>
#include "magic_8_ball_face.h"

#define MAGIC_8_BALL_NUM_DECISIONS 28
#define ANIMATION_TICK_FREQUENCY 16
#define NO_ANIMATION 0
#define SHAKE_ANIMATION 1
#define DECISION_ANIMATION 2

const char* decisions[MAGIC_8_BALL_NUM_DECISIONS][6] = {
    {"   yes", NULL},
    {"no", NULL},
    {"n&aybe", NULL},
    {" try", "AGAIN", NULL},
    {" AsK", " Later", NULL},
    {"no way", NULL},
    {"absolu", " tELy", NULL},
    {" neVer", NULL},
    {" possI", "   bLy", NULL},
    {"It Is", " cErt", "   aIN", NULL},
    {"It Is", " deCI", " dEdly", "    so", NULL},
    {"  WIth", "ou+ a", " doUbt", NULL},
    {"   yes", " dEFI", "nI+ELy", NULL},
    {"   you", "  n&ay", " rELy", " on it", NULL},
    {" As I", "sEE IT", "   yes", NULL},
    {"n&ost", "LIkely", NULL},
    {"  out", " LoOk", " GoOd", NULL},
    {" sIGNs", " poINT", "+o yes", NULL},
    {" rEpLy", "  HA2Y", " Try", "AGAIN", NULL},
    {"BE++Er", "  not", " tELL", "   you", "  now", NULL},
    {"cAnnot", " prE", " dict", "  now", NULL},
    {"concen", " trAtE", " And", " Ask", "AGAIN", NULL},
    {" doNt", " CoUNt", " on it", NULL},
    {"  n&y", " rEpLy", "Is NO", NULL},
    {"  n&y", "sourcE", "  sAy", "no", NULL},
    {"  out", " LoOk", "  not", " GoOd", NULL},
    {"  VEry", " doUbt", " FUL", NULL},
    {"IT IS", "In&pos", " sIbLE", NULL},
};

static void display_decision_animation(magic_8_ball_state_t *state) {
    const char** decision = decisions[state->decision];
    watch_display_string("      ", 4);
    watch_display_string((char*)decision[state->animation_frame], 4);
    state->animation_frame = decision[state->animation_frame + 1] == NULL ? 0 : state->animation_frame + 1;
}

static void make_decision(magic_8_ball_state_t *state) {
    #if __EMSCRIPTEN__
    state->decision = rand() % MAGIC_8_BALL_NUM_DECISIONS;
    #else
    state->decision = arc4random_uniform(MAGIC_8_BALL_NUM_DECISIONS);
    #endif
}

const char* shake_animation_frames[6] = {
    "      ",  // Frame 1: Open parentheses
    "------",  // Frame 2: A character appears in the middle
    "OOOOOO",  // Frame 3: Middle character changes, growing larger
    "888888",  // Frame 4: Same as Frame 3 for consistency
    "OOOOOO",  // Frame 5: Character shrinks back down
    "------",  // Frame 6: Back to open parentheses
};

uint8_t total_shake_animation_frames = sizeof(shake_animation_frames) / sizeof(shake_animation_frames[0]);

static void display_shake_animation(magic_8_ball_state_t *state) {
    watch_display_string((char*)shake_animation_frames[state->animation_frame], 4);
    if (state->animation_frame == 2) {
        watch_buzzer_play_note(BUZZER_NOTE_C7, 50);
        make_decision(state);
    } else if (state->animation_frame == 5) {
        watch_buzzer_play_note(BUZZER_NOTE_C6, 50);
        make_decision(state);
    }
    state->animation_frame = (state->animation_frame + 1) % total_shake_animation_frames;
}

static void display_animation(magic_8_ball_state_t *state) {
    if (state->animation_type == SHAKE_ANIMATION) {
        display_shake_animation(state);
    } else if (state->animation_type == DECISION_ANIMATION) {
        display_decision_animation(state);
    }
}

void magic_8_ball_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(magic_8_ball_state_t));
        memset(*context_ptr, 0, sizeof(magic_8_ball_state_t));
    }
    #if __EMSCRIPTEN__
    srand(time(NULL));
    #endif
}

void magic_8_ball_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    magic_8_ball_state_t *state = (magic_8_ball_state_t *)context;
    state->decision = 0;
    state->animation_frame = 0;
    state->animation_type = NO_ANIMATION;
}

bool magic_8_ball_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    magic_8_ball_state_t *state = (magic_8_ball_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            watch_display_string("M8   yEs? ", 0);
            break;
        case EVENT_TICK:
            if (
                state->animation_type == SHAKE_ANIMATION
                && !watch_get_pin_level(BTN_ALARM)
                && !watch_get_pin_level(BTN_LIGHT)
            ) {
                state->animation_type = DECISION_ANIMATION;
                state->animation_frame = 0;
                movement_request_tick_frequency(1);
            }
            display_animation(state);
            break;
        case EVENT_LIGHT_BUTTON_DOWN:
            movement_illuminate_led();
            // fallthrough
        case EVENT_ALARM_BUTTON_DOWN:
            state->animation_type = SHAKE_ANIMATION;
            movement_request_tick_frequency(ANIMATION_TICK_FREQUENCY);
            break;
        default:
            movement_default_loop_handler(event, settings);
            break;
    }

    return true;
}

void magic_8_ball_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}
