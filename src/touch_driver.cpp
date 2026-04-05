#include "lvgl.h"
#include "touch_bsp.h"
#include "inclinometer_shared.h"

static const uint8_t touchPressDebounceSamples = 2;
static const uint8_t touchReleaseDebounceSamples = 2;

void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    static uint8_t pressed_samples = 0;
    static uint8_t released_samples = 0;
    static bool debounced_pressed = false;
    static lv_point_t last_point = {0, 0};

    if (!getTouchInputEnabled()) {
        pressed_samples = 0;
        released_samples = 0;
        debounced_pressed = false;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x, y;
    const bool raw_pressed = getTouch(&x, &y) != 0;

    if (raw_pressed) {
        released_samples = 0;
        if (pressed_samples < touchPressDebounceSamples) pressed_samples++;
        if (pressed_samples >= touchPressDebounceSamples) {
            debounced_pressed = true;
            last_point.x = x;
            last_point.y = y;
        }
    } else {
        pressed_samples = 0;
        if (released_samples < touchReleaseDebounceSamples) released_samples++;
        if (released_samples >= touchReleaseDebounceSamples) {
            debounced_pressed = false;
        }
    }

    if (debounced_pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point = last_point;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

