/*
 * MIT License
 *
 * Copyright (c) 2022 Joey Castillo
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
#include "preferences_face.h"
#include "watch.h"

#define PREFERENCES_FACE_NUM_PREFERENCES (7)
const char preferences_face_titles[PREFERENCES_FACE_NUM_PREFERENCES][11] = {
    "ST        ",   // Silent time range
    "CL        ",   // Clock: 12 or 24 hour
    "BT  Beep  ",   // Buttons: should they beep?
    "TO        ",   // Timeout: how long before we snap back to the clock face?
    "LE        ",   // Low Energy mode: how long before it engages?
    "LT   grn  ",   // Light: green component
    "LT   red  ",   // Light: red component
};

typedef struct {
    uint8_t current_page;
    uint8_t current_blink;
} preferences_state_t;

void blink_value(char *buffer, int position, int length, int subsecond) {
    char spaces[32] = {0};
    if (subsecond % 2 == 0) {
        for (uint8_t i = 0; i < length && i < sizeof(spaces) - 1; i++) spaces[i] = ' ';
        watch_display_string(spaces, position);
    } else {
        watch_display_string(buffer, position);
    }
}

void preferences_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(preferences_state_t));
        preferences_state_t *state = (preferences_state_t *)*context_ptr;
        state->current_page = 0;
        state->current_blink = 0;
    }
}

void preferences_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    preferences_state_t *state = (preferences_state_t *)context;
    state->current_page = 0;
    movement_request_tick_frequency(4);
}

bool preferences_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    preferences_state_t *state = (preferences_state_t *)context;
    switch (event.event_type) {
        case EVENT_TICK:
        case EVENT_ACTIVATE:
            // Do nothing; handled below.
            break;
        case EVENT_MODE_BUTTON_UP:
            watch_set_led_off();
            movement_move_to_next_face();
            return false;
        case EVENT_LIGHT_BUTTON_DOWN:
            if (state->current_page == 0 && state->current_blink == 0) {
                state->current_blink = 1;
            } else {
                state->current_blink = 0;
                state->current_page = (state->current_page + 1) % PREFERENCES_FACE_NUM_PREFERENCES;
            }
            break;
        case EVENT_ALARM_BUTTON_UP:
                switch (state->current_page) {
                case 0:
                    if (state->current_blink == 0) {
                        settings->bit.silent_from = (settings->bit.silent_from + 1) % 24;
                    } else if (state->current_blink == 1) {
                        settings->bit.silent_to = (settings->bit.silent_to + 1) % 24;
                    }
                    break;
                case 1:
                    settings->bit.clock_mode_24h = !(settings->bit.clock_mode_24h);
                    break;
                case 2:
                    settings->bit.button_should_sound = !(settings->bit.button_should_sound);
                    break;
                case 3:
                    settings->bit.to_interval = settings->bit.to_interval + 1;
                    break;
                case 4:
                    settings->bit.le_interval = settings->bit.le_interval + 1;
                    break;
                case 5:
                    settings->bit.led_green_color = settings->bit.led_green_color + 1;
                    break;
                case 6:
                    settings->bit.led_red_color = settings->bit.led_red_color + 1;
                    break;
            }
            break;
        case EVENT_TIMEOUT:
            movement_move_to_face(0);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    watch_display_string((char *)preferences_face_titles[state->current_page], 0);

    char buf[8];
    switch (state->current_page) {
        case 0:
            if (state->current_blink == 0) {
                sprintf(buf, "%2d", settings->bit.silent_from);
                blink_value(buf, 4, 2, event.subsecond);
                sprintf(buf, "%2d", settings->bit.silent_to);
                watch_display_string(buf, 6);
            } else if (state->current_blink == 1) {
                sprintf(buf, "%2d", settings->bit.silent_from);
                watch_display_string(buf, 4);
                sprintf(buf, "%2d", settings->bit.silent_to);
                blink_value(buf, 6, 2, event.subsecond);
            }
            break;
        case 1:
            if (settings->bit.clock_mode_24h) blink_value("24h", 4, 3, event.subsecond);
            else blink_value("12h", 4, 3, event.subsecond);
            break;
        case 2:
            if (settings->bit.button_should_sound) blink_value("y", 9, 1, event.subsecond);
            else blink_value("n", 9, 1, event.subsecond);
            break;
        case 3:
            switch (settings->bit.to_interval) {
                case 0:
                    blink_value("60 SeC", 4, 6, event.subsecond);
                    break;
                case 1:
                    blink_value("2 n&in", 4, 6, event.subsecond);
                    break;
                case 2:
                    blink_value("5 n&in", 4, 6, event.subsecond);
                    break;
                case 3:
                    blink_value("30n&in", 4, 6, event.subsecond);
                    break;
            }
            break;
        case 4:
            switch (settings->bit.le_interval) {
                case 0:
                    blink_value(" Never", 4, 6, event.subsecond);
                    break;
                case 1:
                    blink_value("10n&in", 4, 6, event.subsecond);
                    break;
                case 2:
                    blink_value("1 hour", 4, 6, event.subsecond);
                    break;
                case 3:
                    blink_value("2 hour", 4, 6, event.subsecond);
                    break;
                case 4:
                    blink_value("6 hour", 4, 6, event.subsecond);
                    break;
                case 5:
                    blink_value("12 hr ", 4, 6, event.subsecond);
                    break;
                case 6:
                    blink_value(" 1 day", 4, 6, event.subsecond);
                    break;
                case 7:
                    blink_value(" 7 day", 4, 6, event.subsecond);
                    break;
            }
            break;
        case 5:
            sprintf(buf, "%2d", settings->bit.led_green_color);
            blink_value(buf, 8, 2, event.subsecond);
            break;
        case 6:
            sprintf(buf, "%2d", settings->bit.led_red_color);
            blink_value(buf, 8, 2, event.subsecond);
            break;
    }

    if (state->current_page >= 5) {
        watch_set_led_color(settings->bit.led_red_color ? (0xF | settings->bit.led_red_color << 4) : 0,
                            settings->bit.led_green_color ? (0xF | settings->bit.led_green_color << 4) : 0);
        // return false so the watch stays awake (needed for the PWM driver to function).
        return false;
    }

    watch_set_led_off();
    return true;
}

void preferences_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    preferences_state_t *state = (preferences_state_t *)context;
    state->current_page = 0;
    state->current_blink = 0;
    watch_set_led_off();
    watch_store_backup_data(settings->reg, 0);
}
