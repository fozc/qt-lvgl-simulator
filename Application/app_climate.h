#ifndef APP_CLIMATE_H
#define APP_CLIMATE_H

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

void app_climate_set_time(uint32_t unix_seconds, int16_t utc_offset_minutes);
void app_climate_show(lv_obj_t * return_screen);

#ifdef __cplusplus
}
#endif

#endif