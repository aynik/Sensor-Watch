#include <stdlib.h>
#include <string.h>
#include "signal_face.h"

static void _signal_face_update_display(int detection_count) {
    char buf[11];
    sprintf(buf, "SI    %.2d", detection_count);
    watch_display_string(buf, 0);
}

void signal_face_setup(movement_settings_t *settings, uint8_t watch_face_index, void **context_ptr) {
    (void) settings;
    (void) watch_face_index;
    if (*context_ptr == NULL) {
        *context_ptr = malloc(sizeof(signal_state_t));
        memset(*context_ptr, 0, sizeof(signal_state_t));
        signal_state_t *state = (signal_state_t *)*context_ptr;
        state->prev_green_state = false;
        state->detection_count = 0;
    }
}

void signal_face_activate(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_set_led_in();
    movement_request_tick_frequency(2);
}

bool signal_face_loop(movement_event_t event, movement_settings_t *settings, void *context) {
    (void) settings;
    signal_state_t *state = (signal_state_t *)context;
    bool current_green_state = state->prev_green_state;

    switch (event.event_type) {
    case EVENT_ACTIVATE:
        state->prev_green_state = watch_read_green_led_state();
        _signal_face_update_display(state->detection_count);
        break;
    case EVENT_TICK:
        current_green_state = watch_read_green_led_state();

        if (current_green_state != state->prev_green_state) {
            state->detection_count++;
        }

        state->prev_green_state = current_green_state;
        _signal_face_update_display(state->detection_count);
        break;
    default:
        movement_default_loop_handler(event, settings);
        break;
    }

    return true;
}

void signal_face_resign(movement_settings_t *settings, void *context) {
    (void) settings;
    (void) context;
    watch_set_led_out();
}
