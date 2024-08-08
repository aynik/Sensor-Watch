/*
 * MIT License
 *
 * Copyright (c) 2022 Andreas Nebinger
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

#include "tune_alarm_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "watch_private_display.h"
#include "movement_custom_signal_tunes.h"

typedef enum {
    alarm_setting_idx_alarm,
    alarm_setting_idx_day,
    alarm_setting_idx_hour,
    alarm_setting_idx_minute,
    alarm_setting_idx_tune
} alarm_setting_idx_t;

static const char _dow_strings[TUNE_ALARM_DAY_STATES + 1][2] ={"AL", "MO", "TU", "WE", "TH", "FR", "SA", "SO", "ED", "1t", "MF", "WN"};
static const uint8_t _blink_idx[TUNE_ALARM_SETTING_STATES] = {2, 0, 4, 6, 8, 10};
static const uint8_t _blink_idx2[TUNE_ALARM_SETTING_STATES] = {3, 1, 5, 7, 9, 11};

static int8_t _wait_ticks;

static uint8_t _get_weekday_idx(watch_date_time date_time) {
    date_time.unit.year += 20;
    if (date_time.unit.month <= 2) {
        date_time.unit.month += 12;
        date_time.unit.year--;
    }
    return (date_time.unit.day + 13 * (date_time.unit.month + 1) / 5 + date_time.unit.year + date_time.unit.year / 4 + 525 - 2) % 7;
}

static void _alarm_set_signal(tune_alarm_state_t *state) {
    if (state->tune_alarm[state->tune_alarm_idx].enabled)
        watch_set_indicator(WATCH_INDICATOR_SIGNAL);
    else
        watch_clear_indicator(WATCH_INDICATOR_SIGNAL);
}

static void _tune_alarm_face_draw(movement_settings_t *settings, tune_alarm_state_t *state, uint8_t subsecond) {
    char buf[12];

    uint8_t i = 0;
    if (state->is_setting) {
        i = state->tune_alarm[state->tune_alarm_idx].day + 1;
    }
    uint8_t h = state->tune_alarm[state->tune_alarm_idx].hour;
    if (!settings->bit.clock_mode_24h) {
        if (h >= 12) {
            watch_set_indicator(WATCH_INDICATOR_PM);
            h %= 12;
        } else {
            watch_clear_indicator(WATCH_INDICATOR_PM);
        }
        if (h == 0) h = 12;
    }
    sprintf(buf, "%c%c%2d%2d%02d%2d",
        _dow_strings[i][0], _dow_strings[i][1],
        (state->tune_alarm_idx + 1),
        h,
        state->tune_alarm[state->tune_alarm_idx].minute,
        state->tune_alarm[state->tune_alarm_idx].tune_idx);
    if (state->is_setting && subsecond % 2 && state->setting_state <= alarm_setting_idx_tune && !state->tune_alarm_quick_ticks) {
        buf[_blink_idx[state->setting_state]] = buf[_blink_idx2[state->setting_state]] = ' ';
    }
    watch_display_string(buf, 0);

    _alarm_set_signal(state);
}

static void _alarm_initiate_setting(movement_settings_t *settings, tune_alarm_state_t *state, uint8_t subsecond) {
    state->is_setting = true;
    state->setting_state = 0;
    movement_request_tick_frequency(4);
    _tune_alarm_face_draw(settings, state, subsecond);
}

static void _alarm_resume_setting(movement_settings_t *settings, tune_alarm_state_t *state, uint8_t subsecond) {
    state->is_setting = false;
    movement_request_tick_frequency(1);
    _tune_alarm_face_draw(settings, state, subsecond);
}

static void _alarm_update_alarm_enabled(movement_settings_t *settings, tune_alarm_state_t *state) {
    bool active_alarms = false;
    watch_date_time now;
    bool now_init = false;
    uint8_t weekday_idx;
    uint16_t now_minutes_of_day;
    uint16_t alarm_minutes_of_day;
    for (uint8_t i = 0; i < TUNE_ALARM_TUNE_ALARMS; i++) {
        if (state->tune_alarm[i].enabled) {
            if (state->tune_alarm[i].day == TUNE_ALARM_DAY_EACH_DAY || state->tune_alarm[i].day == TUNE_ALARM_DAY_ONE_TIME) {
                active_alarms = true;
                break;
            } else {
                if (!now_init) {
                    now = watch_rtc_get_date_time();
                    now_init = true;
                    weekday_idx = _get_weekday_idx(now);
                    now_minutes_of_day = now.unit.hour * 60 + now.unit.minute;
                }
                alarm_minutes_of_day = state->tune_alarm[i].hour * 60 + state->tune_alarm[i].minute;
                if ((state->tune_alarm[i].day == weekday_idx && alarm_minutes_of_day >= now_minutes_of_day)
                    || ((weekday_idx + 1) % 7 == state->tune_alarm[i].day && alarm_minutes_of_day <= now_minutes_of_day)
                    || (state->tune_alarm[i].day == TUNE_ALARM_DAY_WORKDAY && (weekday_idx < 4
                        || (weekday_idx == 4 && alarm_minutes_of_day >= now_minutes_of_day)
                        || (weekday_idx == 6 && alarm_minutes_of_day <= now_minutes_of_day)))
                    || (state->tune_alarm[i].day == TUNE_ALARM_DAY_WEEKEND && (weekday_idx == 5
                        || (weekday_idx == 6 && alarm_minutes_of_day >= now_minutes_of_day)
                        || (weekday_idx == 4 && alarm_minutes_of_day <= now_minutes_of_day)))) {
                    active_alarms = true;
                    break;
                }
            }
        }
    }
    settings->bit.alarm_enabled = active_alarms;
}

static void _abort_quick_ticks(tune_alarm_state_t *state) {
    if (state->tune_alarm_quick_ticks) {
        state->tune_alarm[state->tune_alarm_idx].enabled = true;
        state->tune_alarm_quick_ticks = false;
        movement_request_tick_frequency(4);
    }
}

static void _play_tune_preview(uint8_t tune_idx) {
    if (tune_idx == 0) {
        watch_buzzer_play_sequence(signal_tunes[rand() % NUM_TUNES], NULL);
    } else {
        watch_buzzer_play_sequence(signal_tunes[tune_idx - 1], NULL);
    }
}

static void _play_tune_alarm(uint8_t tune_idx) {
    if (tune_idx == 0) {
        movement_play_alarm_tune(signal_tunes[rand() % NUM_TUNES]);
    } else {
        movement_play_alarm_tune(signal_tunes[tune_idx - 1]);
    }
}

void tune_alarm_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;

    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(tune_alarm_state_t));
        tune_alarm_state_t *state = (tune_alarm_state_t *)*context_ptr;
        memset(*context_ptr, 0, sizeof(tune_alarm_state_t));
        for (uint8_t i = 0; i < TUNE_ALARM_TUNE_ALARMS; i++) {
            state->tune_alarm[i].day = TUNE_ALARM_DAY_EACH_DAY;
        }
        state->tune_alarm_handled_minute = -1;
        _wait_ticks = -1;
        _alarm_update_alarm_enabled(settings, state);
    }
}

void tune_alarm_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_set_colon();
}

void tune_alarm_face_resign(movement_settings_t *settings, void *context) {
    tune_alarm_state_t *state = (tune_alarm_state_t *)context;
    state->is_setting = false;
    watch_buzzer_abort_sequence();
    _alarm_update_alarm_enabled(settings, state);
    watch_set_led_off();
    state->tune_alarm_quick_ticks = false;
    _wait_ticks = -1;
    movement_request_tick_frequency(1);
}

bool tune_alarm_face_wants_background_task(movement_settings_t *settings, void *context) {
    (void) settings;
    tune_alarm_state_t *state = (tune_alarm_state_t *)context;
    watch_date_time now = watch_rtc_get_date_time();
    if (state->tune_alarm_handled_minute == now.unit.minute) return false;
    state->tune_alarm_handled_minute = now.unit.minute;
    for (uint8_t i = 0; i < TUNE_ALARM_TUNE_ALARMS; i++) {
        if (state->tune_alarm[i].enabled) {
            if (state->tune_alarm[i].minute == now.unit.minute) {
                if (state->tune_alarm[i].hour == now.unit.hour) {
                    state->tune_alarm_idx = i;
                    if (state->tune_alarm[i].day == TUNE_ALARM_DAY_EACH_DAY || state->tune_alarm[i].day == TUNE_ALARM_DAY_ONE_TIME) return true;
                    uint8_t weekday_idx = _get_weekday_idx(now);
                    if (state->tune_alarm[i].day == weekday_idx) return true;
                    if (state->tune_alarm[i].day == TUNE_ALARM_DAY_WORKDAY && weekday_idx < 5) return true;
                    if (state->tune_alarm[i].day == TUNE_ALARM_DAY_WEEKEND && weekday_idx >= 5) return true;
                }
            }
        }
    }
    state->tune_alarm_handled_minute = -1;
    if (now.unit.minute % 12 == 0) _alarm_update_alarm_enabled(settings, state);
    return false;
}

bool tune_alarm_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    tune_alarm_state_t *state = (tune_alarm_state_t *)context;

    switch (event.event_type) {
    case EVENT_TICK:
        if (state->tune_alarm_quick_ticks) {
            if (state->setting_state == alarm_setting_idx_hour) {
                state->tune_alarm[state->tune_alarm_idx].hour = (state->tune_alarm[state->tune_alarm_idx].hour + 1) % 24;
            } else if (state->setting_state == alarm_setting_idx_minute) {
                state->tune_alarm[state->tune_alarm_idx].minute = (state->tune_alarm[state->tune_alarm_idx].minute + 1) % 60;
            } else if (state->setting_state == alarm_setting_idx_tune) {
                watch_buzzer_abort_sequence();
                state->tune_alarm[state->tune_alarm_idx].tune_idx = (state->tune_alarm[state->tune_alarm_idx].tune_idx + 1) % (NUM_TUNES + 1);
            } else _abort_quick_ticks(state);
        } else if (!state->is_setting) {
            if (_wait_ticks >= 0) _wait_ticks++;
            if (_wait_ticks == 2) {
                _wait_ticks = -1;
                if (state->tune_alarm_idx) {
                    state->tune_alarm[state->tune_alarm_idx].enabled ^= 1;
                    _alarm_set_signal(state);
                    delay_ms(275);
                    state->tune_alarm_idx = 0;
                }
            } else break;
        }
        // fallthrough
    case EVENT_ACTIVATE:
        _tune_alarm_face_draw(settings, state, event.subsecond);
        break;
    case EVENT_LIGHT_BUTTON_UP:
        if (!state->is_setting) {
            movement_illuminate_led();
            _alarm_initiate_setting(settings, state, event.subsecond);
            break;
        }
        state->setting_state += 1;
        if (state->setting_state == alarm_setting_idx_tune) {
            _play_tune_preview(state->tune_alarm[state->tune_alarm_idx].tune_idx);
        } else if (state->setting_state >= alarm_setting_idx_tune + 1) {
            _alarm_resume_setting(settings, state, event.subsecond);
        }
        break;
    case EVENT_LIGHT_LONG_PRESS:
        if (state->is_setting) {
            _alarm_resume_setting(settings, state, event.subsecond);
        } else {
            _alarm_initiate_setting(settings, state, event.subsecond);
        }
        break;
    case EVENT_ALARM_BUTTON_UP:
        if (!state->is_setting) {
            _wait_ticks = -1;
            state->tune_alarm_idx = (state->tune_alarm_idx + 1) % (TUNE_ALARM_TUNE_ALARMS);
        } else {
            switch (state->setting_state) {
            case alarm_setting_idx_alarm:
                state->tune_alarm_idx = (state->tune_alarm_idx + 1) % (TUNE_ALARM_TUNE_ALARMS);
                break;
            case alarm_setting_idx_day:
                state->tune_alarm[state->tune_alarm_idx].day = (state->tune_alarm[state->tune_alarm_idx].day + 1) % (TUNE_ALARM_DAY_STATES);
                break;
            case alarm_setting_idx_hour:
                _abort_quick_ticks(state);
                state->tune_alarm[state->tune_alarm_idx].hour = (state->tune_alarm[state->tune_alarm_idx].hour + 1) % 24;
                break;
            case alarm_setting_idx_minute:
                _abort_quick_ticks(state);
                state->tune_alarm[state->tune_alarm_idx].minute = (state->tune_alarm[state->tune_alarm_idx].minute + 1) % 60;
                break;
            case alarm_setting_idx_tune:
                _abort_quick_ticks(state);
                state->tune_alarm[state->tune_alarm_idx].tune_idx = (state->tune_alarm[state->tune_alarm_idx].tune_idx + 1) % (NUM_TUNES + 1);
                _play_tune_preview(state->tune_alarm[state->tune_alarm_idx].tune_idx);
                break;
            default:
                break;
            }
            if (state->setting_state > alarm_setting_idx_alarm) state->tune_alarm[state->tune_alarm_idx].enabled = true;
        }
        _tune_alarm_face_draw(settings, state, event.subsecond);
        break;
    case EVENT_ALARM_LONG_PRESS:
        if (!state->is_setting) {
            state->tune_alarm[state->tune_alarm_idx].enabled ^= 1;
            _wait_ticks = 0;
        } else {
            switch (state->setting_state) {
            case alarm_setting_idx_alarm:
                state->tune_alarm_idx = 0;
                break;
            case alarm_setting_idx_tune:
            case alarm_setting_idx_minute:
            case alarm_setting_idx_hour:
                movement_request_tick_frequency(8);
                state->tune_alarm_quick_ticks = true;
                break;
            default:
                break;
            }
        }
        _tune_alarm_face_draw(settings, state, event.subsecond);
        break;
    case EVENT_ALARM_LONG_UP:
        if (state->is_setting) {
            if (
                state->setting_state == alarm_setting_idx_hour
                || state->setting_state == alarm_setting_idx_minute
                || state->setting_state == alarm_setting_idx_tune
            ) _abort_quick_ticks(state);
            if (state->setting_state == alarm_setting_idx_tune) {
                _play_tune_preview(state->tune_alarm[state->tune_alarm_idx].tune_idx);
            }
        } else _wait_ticks = -1;
        break;
    case EVENT_BACKGROUND_TASK:
        watch_buzzer_abort_sequence();
        movement_move_to_face_silently(1);
        _play_tune_alarm(state->tune_alarm[state->tune_alarm_idx].tune_idx);
        if (state->tune_alarm[state->tune_alarm_idx].day == TUNE_ALARM_DAY_ONE_TIME) {
            state->tune_alarm[state->tune_alarm_idx].day = TUNE_ALARM_DAY_EACH_DAY;
            state->tune_alarm[state->tune_alarm_idx].minute = 0;
            state->tune_alarm[state->tune_alarm_idx].hour = 0;
            state->tune_alarm[state->tune_alarm_idx].enabled = false;
            _alarm_update_alarm_enabled(settings, state);
        }
        break;
    case EVENT_TIMEOUT:
        movement_move_to_face(0);
        break;
    case EVENT_LIGHT_BUTTON_DOWN:
    case EVENT_ALARM_BUTTON_DOWN:
        watch_buzzer_abort_sequence();
        break;
    default:
        movement_default_loop_handler(event, settings);
        break;
    }

    return true;
}
