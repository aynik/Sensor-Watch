/*
 * MIT License
 *
 * Copyright (c) 2023 Austin Teets
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

#include <stdlib.h>
#include <string.h>
#include "metronome_face.h"
#include "watch.h"

static int8_t sound_seq_start[] = {BUZZER_NOTE_C8, 2, 0};
static int8_t sound_seq_beat[] = {BUZZER_NOTE_C6, 2, 0};

void metronome_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(metronome_state_t));
        memset(*context_ptr, 0, sizeof(metronome_state_t));
    }
}

void metronome_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    metronome_state_t *state = (metronome_state_t *)context;
    movement_request_tick_frequency(4);
    if (state->bpm == 0) {
        state->count = 4;
        state->bpm = 120;
    }
    state->mode = MODE_WAIT;
    state->setting = SETTING_COUNT;
    state->quick_ticks_running = false;
}

static void update_lcd(metronome_state_t *state) {
    char buf[11];
    sprintf(buf, "MN%2hhu %3hhu%s", state->count, state->bpm, "bp");
    watch_display_string(buf, 0);
}

static void start_stop(metronome_state_t *state) {
    if (state->mode != MODE_RUN) {
        movement_request_tick_frequency(64);
        state->mode = MODE_RUN;
        state->tick = 3840 / state->bpm;
        state->cur_tick = state->tick;
        state->cur_beat = 1;
    } else {
        state->mode = MODE_WAIT;
        movement_request_tick_frequency(4);
        update_lcd(state);
    }
}

static void display_tick(metronome_state_t *state, uint8_t beat) {
    char buf[11];
    sprintf(buf, "MN%2hhu %3hhu%s", state->count, state->bpm, beat > 0 ? "  " : "bp");
    watch_display_string(buf, 0);
}

static void tick_beat(metronome_state_t *state, uint8_t beat) {
    watch_buzzer_play_sequence(state->cur_beat == 1 ? sound_seq_start : sound_seq_beat, NULL);
    display_tick(state, beat);
}

static void event_tick(metronome_state_t *state) {
    if (state->cur_tick == 0) {
        tick_beat(state, 0);
        state->cur_tick = state->tick;
        state->cur_beat = (state->cur_beat < state->count) ? state->cur_beat + 1 : 1;
    } else {
        if (state->cur_tick == state->tick / 2) {
            display_tick(state, 1);
        }
        state->cur_tick--;
    }
}

static void setting_tick(uint8_t subsecond, metronome_state_t *state) {
    char buf[11];
    sprintf(buf, "MN%2hhu %3hhu%s", state->count, state->bpm, "bp");

    if (subsecond % 2 == 0 && !state->quick_ticks_running) {
        if (state->setting == SETTING_COUNT) {
            buf[2] = ' ';
            buf[3] = ' ';
        } else {
            buf[5] = ' ';
            buf[6] = ' ';
            buf[7] = ' ';
        }
    }
    watch_display_string(buf, 0);
}

static void update_setting(metronome_state_t *state, bool increment) {
    if (state->setting == SETTING_COUNT) {
        state->count += increment ? 1 : -1;
        if (state->count < 1) state->count = 16;
        if (state->count > 16) state->count = 1;
    } else {
        state->bpm += increment ? 1 : -1;
        if (state->bpm < 1) state->bpm = 240;
        if (state->bpm > 240) state->bpm = 1;
    }
}

static void abort_quick_ticks(metronome_state_t *state) {
    if (state->quick_ticks_running) {
        state->quick_ticks_running = false;
        movement_request_tick_frequency(4);
    }
}

bool metronome_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    metronome_state_t *state = (metronome_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
            update_lcd(state);
            break;
        case EVENT_TICK:
            if (state->mode == MODE_RUN) {
                event_tick(state);
            } else if (state->mode == MODE_SET) {
                setting_tick(event.subsecond, state);
                if (state->quick_ticks_running) {
                    if (watch_get_pin_level(BTN_ALARM)) {
                        update_setting(state, true);
                    } else if (watch_get_pin_level(BTN_LIGHT)) {
                        update_setting(state, false);
                    } else {
                        abort_quick_ticks(state);
                    }
                }
            }
            break;
        case EVENT_LIGHT_BUTTON_UP:
            if (state->mode == MODE_SET) {
                if (state->setting == SETTING_COUNT) {
                    state->setting = SETTING_BPM;
                } else {
                    state->mode = MODE_WAIT;
                    update_lcd(state);
                }
            } else {
                movement_request_tick_frequency(4);
                state->mode = MODE_SET;
                state->setting = SETTING_COUNT;
                update_lcd(state);
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
            if (state->mode == MODE_SET) {
                update_setting(state, true);
            } else {
                start_stop(state);
            }
            break;
        case EVENT_ALARM_LONG_PRESS:
        case EVENT_LIGHT_LONG_PRESS:
            if (state->mode == MODE_SET) {
                state->quick_ticks_running = true;
                movement_request_tick_frequency(8);
            }
            break;
        case EVENT_ALARM_LONG_UP:
        case EVENT_LIGHT_LONG_UP:
            abort_quick_ticks(state);
            break;
        case EVENT_MODE_BUTTON_UP:
            abort_quick_ticks(state);
            if (state->mode == MODE_SET || state->mode == MODE_RUN) {
                state->mode = MODE_WAIT;
                update_lcd(state);
            } else {
                movement_move_to_next_face();
            }
            break;
        case EVENT_TIMEOUT:
            abort_quick_ticks(state);
            if (state->mode != MODE_RUN) {
                movement_move_to_face(0);
            }
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }
    return true;
}

void metronome_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}