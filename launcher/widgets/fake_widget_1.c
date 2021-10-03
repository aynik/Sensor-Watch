#include "fake_widget_1.h"
#include "watch.h"

void fake_widget_1_setup(LauncherSettings *settings, void ** context_ptr) {
    (void) settings;
    *context_ptr = NULL;
}

void fake_widget_1_activate(LauncherSettings *settings, void *context) {
    (void) settings;
    (void) context;
}

void fake_widget_1_loop(LauncherEvent event, LauncherSettings *settings, uint8_t subsecond, void *context) {
    printf("fake_widget_1_loop\n");
    (void) settings;
    (void) subsecond;
    (void) context;
    watch_display_string("W1 d get01", 0);

    switch (event) {
        case EVENT_MODE_BUTTON_UP:
            launcher_move_to_next_widget();
            return;
        case EVENT_LIGHT_BUTTON_UP:
            launcher_illuminate_led();
            break;
        default:
            break;
    }
}

void fake_widget_1_resign(LauncherSettings *settings, void *context) {
    (void) settings;
    (void) context;
}
