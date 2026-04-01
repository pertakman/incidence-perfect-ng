#include "lvgl.h"
#include "touch_bsp.h"
#include "inclinometer_shared.h"

void touch_read_cb(lv_indev_drv_t *drv, lv_indev_data_t *data)
{
    (void)drv;
    if (!getTouchInputEnabled()) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint16_t x, y;

    if (getTouch(&x, &y)) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

