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
#include <math.h>
#include "simple_sunrise_sunset_face.h"
#include "watch.h"
#include "watch_utility.h"
#include "sunriset.h"

static void draw_calculation_segments(uint8_t current_calculation) {
    const uint8_t clear_pixels[][2] = {{2, 2}, {2, 3}, {0, 4}, {0, 5}, {1, 4}, {2, 4}, {1, 2}, {0, 2}, {0, 3}, {0, 6}, {1, 6}, {2, 5}};
    const uint8_t segment_pixels[][8][2] = {
        {{2, 3}, {0, 4}, {0, 5}, {1, 4}}, // CALCULATION_DAY_LENGTH
        {{0, 3}}, // CALCULATION_ASTRONOMICAL_DAWN
        {{0, 3}, {0, 2}}, // CALCULATION_NAUTICAL_DAWN
        {{0, 3}, {0, 2}, {1, 2}}, // CALCULATION_CIVIL_DAWN
        {{0, 3}, {0, 2}, {1, 2}, {2, 2}}, // CALCULATION_SUNRISE
        {{0, 4}, {0, 5}}, // CALCULATION_SOLAR_NOON
        {{2, 4}, {2, 5}, {1, 6}, {0, 6}}, // CALCULATION_SUNSET
        {{2, 5}, {1, 6}, {0, 6}}, // CALCULATION_CIVIL_DUSK
        {{1, 6}, {0, 6}}, // CALCULATION_NAUTICAL_DUSK
        {{0, 6}}, // CALCULATION_ASTRONOMICAL_DUSK
        {{1, 2}, {0, 2}, {0, 3}, {2, 2}, {0, 6}, {2, 4}, {1, 6}, {2, 5}} // CALCULATION_NIGHT_LENGTH
    };

    for (int i = 0; i < 12; i++) {
        watch_clear_pixel(clear_pixels[i][0], clear_pixels[i][1]);
    }

    for (int i = 0; i < 8; i++) {
        if (segment_pixels[current_calculation][i][0] == 0 && segment_pixels[current_calculation][i][1] == 0) break;
        watch_set_pixel(segment_pixels[current_calculation][i][0], segment_pixels[current_calculation][i][1]);
    }
}

static void display_time(uint8_t current_calculation, int hour, int minute, bool is_24h) {
    char buf[9];
    const char calculation_prefixes[][2] = {"11", "1a", "1n", "1c", "1s", "12", "2s", "2c", "2n", "2a", "22"};
    if (!is_24h) {
        watch_clear_indicator(WATCH_INDICATOR_24H);
        if (hour >= 12) watch_set_indicator(WATCH_INDICATOR_PM);
        else watch_clear_indicator(WATCH_INDICATOR_PM);
        hour = hour % 12;
        if (hour == 0) hour = 12;
    } else {
        watch_clear_indicator(WATCH_INDICATOR_PM);
        watch_set_indicator(WATCH_INDICATOR_24H);
    }
    sprintf(buf, "%.2s%2d%02d", calculation_prefixes[current_calculation], hour, minute);
    watch_set_colon();
    watch_display_string(buf, 2);
    draw_calculation_segments(current_calculation);
}

static void calculate_and_display_sun_time(movement_settings_t *settings, simple_sunrise_sunset_state_t *state) {
    movement_location_t movement_location = (movement_location_t)watch_get_backup_data(1);
    if (movement_location.reg == 0) {
        watch_display_string("dl  no Loc", 0);
        return;
    }

    watch_date_time date_time = watch_rtc_get_date_time();

    int16_t lat_centi = (int16_t)movement_location.bit.latitude;
    int16_t lon_centi = (int16_t)movement_location.bit.longitude;
    double lat = (double)lat_centi / 100.0;
    double lon = (double)lon_centi / 100.0;
    double time_offset = ((double)movement_timezone_offsets[settings->bit.time_zone]) / 60.0;

    uint16_t year = date_time.unit.year + WATCH_RTC_REFERENCE_YEAR;
    uint8_t month = date_time.unit.month;
    uint8_t day = date_time.unit.day;

    double start, end, rise, set;
    double event_time = 0;

    switch(state->current_calculation) {
    case CALCULATION_SUNRISE:
    case CALCULATION_SUNSET:
    case CALCULATION_SOLAR_NOON:
        sun_rise_set(year, month, day, lon, lat, &rise, &set);
        if (state->current_calculation == CALCULATION_SUNRISE) {
            event_time = rise;
        } else if (state->current_calculation == CALCULATION_SUNSET) {
            event_time = set;
        } else {
            event_time = (rise + set) / 2;
        }
        break;
    case CALCULATION_CIVIL_DAWN:
    case CALCULATION_CIVIL_DUSK:
        civil_twilight(year, month, day, lon, lat, &start, &end);
        if (state->current_calculation == CALCULATION_CIVIL_DAWN) {
            event_time = start;
        } else {
            event_time = end;
        }
        break;
    case CALCULATION_NAUTICAL_DAWN:
    case CALCULATION_NAUTICAL_DUSK:
        nautical_twilight(year, month, day, lon, lat, &start, &end);
        if (state->current_calculation == CALCULATION_NAUTICAL_DAWN) {
            event_time = start;
        } else {
            event_time = end;
        }
        break;
    case CALCULATION_ASTRONOMICAL_DAWN:
    case CALCULATION_ASTRONOMICAL_DUSK:
        astronomical_twilight(year, month, day, lon, lat, &start, &end);
        if (state->current_calculation == CALCULATION_ASTRONOMICAL_DAWN) {
            event_time = start;
        } else {
            event_time = end;
        }
        break;
    case CALCULATION_DAY_LENGTH:
    case CALCULATION_NIGHT_LENGTH:
        if (state->current_calculation == CALCULATION_DAY_LENGTH) {
            event_time = day_length(year, month, day, lon, lat);
        } else {
            event_time = 24.0 - day_length(year, month, day, lon, lat);
        }
        break;
    }


    if (state->current_calculation != CALCULATION_DAY_LENGTH && state->current_calculation != CALCULATION_NIGHT_LENGTH) {
        event_time = (event_time + time_offset);
    }

    watch_display_string("SU", 0);

    display_time(
        state->current_calculation,
        floor(event_time),
        floor(fmod(event_time, 1) * 60),
        settings->bit.clock_mode_24h);
}



void simple_sunrise_sunset_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(simple_sunrise_sunset_state_t));
        simple_sunrise_sunset_state_t *state = (simple_sunrise_sunset_state_t *)*context_ptr;
        state->current_calculation = 0;
    }
}

void simple_sunrise_sunset_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
}

bool simple_sunrise_sunset_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    simple_sunrise_sunset_state_t *state = (simple_sunrise_sunset_state_t *)context;

    switch (event.event_type) {
        case EVENT_ACTIVATE:
        case EVENT_TICK:
        case EVENT_LOW_ENERGY_UPDATE:
            calculate_and_display_sun_time(settings, state);
            break;
        case EVENT_LIGHT_BUTTON_UP:
            state->current_calculation = (state->current_calculation - 1 + TOTAL_SUNRISE_SUNSET_MODES) % TOTAL_SUNRISE_SUNSET_MODES;
            calculate_and_display_sun_time(settings, state);
            break;
        case EVENT_ALARM_BUTTON_UP:
            state->current_calculation = (state->current_calculation + 1) % TOTAL_SUNRISE_SUNSET_MODES;
            calculate_and_display_sun_time(settings, state);
            break;
        default:
            return movement_default_loop_handler(event, settings);
    }

    return true;
}

void simple_sunrise_sunset_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    simple_sunrise_sunset_state_t *state = (simple_sunrise_sunset_state_t *)context;
    state->current_calculation = CALCULATION_DAY_LENGTH;
    watch_clear_colon();
    watch_clear_indicator(WATCH_INDICATOR_PM);
    watch_clear_indicator(WATCH_INDICATOR_24H);
}
