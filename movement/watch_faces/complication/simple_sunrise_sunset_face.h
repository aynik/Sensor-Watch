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

#ifndef SIMPLE_SUNRISE_SUNSET_FACE_H_
#define SIMPLE_SUNRISE_SUNSET_FACE_H_

/*
 * SIMPLE UNRISE & SUNSET FACE
 *
 * Like the Sunrise/Sunset face but there's no support for setting the location,
 * or for using multiple locations.
 */

#include "movement.h"

typedef enum {
    CALCULATION_DAY_LENGTH,
    CALCULATION_ASTRONOMICAL_DAWN,
    CALCULATION_NAUTICAL_DAWN,
    CALCULATION_CIVIL_DAWN,
    CALCULATION_SUNRISE,
    CALCULATION_SOLAR_NOON,
    CALCULATION_SUNSET,
    CALCULATION_CIVIL_DUSK,
    CALCULATION_NAUTICAL_DUSK,
    CALCULATION_ASTRONOMICAL_DUSK,
    CALCULATION_NIGHT_LENGTH,
} solar_calculation;

#define TOTAL_SUNRISE_SUNSET_MODES 11

typedef struct {
    solar_calculation current_calculation;
} simple_sunrise_sunset_state_t;

void simple_sunrise_sunset_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void ** context_ptr);
void simple_sunrise_sunset_face_activate(movement_settings_t *settings, void *context);
bool simple_sunrise_sunset_face_loop(movement_event_t event, movement_settings_t *settings, void *context);
void simple_sunrise_sunset_face_resign(movement_settings_t *settings, void *context);

#define simple_sunrise_sunset_face ((const watch_face_t){ \
    simple_sunrise_sunset_face_setup, \
    simple_sunrise_sunset_face_activate, \
    simple_sunrise_sunset_face_loop, \
    simple_sunrise_sunset_face_resign, \
    NULL, \
})

#endif // SIMPLE_SUNRISE_SUNSET_FACE_H
