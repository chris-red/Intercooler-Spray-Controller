#pragma once

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "lvgl.h"
#include "demos/lv_demos.h"

#include "ST7701S.h"
#include "CST820.h"

#define EXAMPLE_LVGL_TICK_PERIOD_MS    2

extern lv_disp_draw_buf_t disp_buf; // contains internal graphic buffer(s) called draw buffer(s)
extern lv_disp_drv_t disp_drv;      // contains callback functions
extern lv_disp_t *disp;
void example_lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);
void example_increase_lvgl_tick(void *arg);
/*Read the touchpad*/
void example_touchpad_read( lv_indev_drv_t * drv, lv_indev_data_t * data );

void LVGL_Init(void);

/**
 * Lock the LVGL mutex. Must be called before any LVGL API usage from
 * tasks other than the one running lv_timer_handler().
 * @param timeout_ms Timeout in ms (0 = wait forever)
 * @return true if the lock was acquired
 */
bool lvgl_port_lock(uint32_t timeout_ms);

/**
 * Unlock the LVGL mutex. Must be called after LVGL API usage is complete.
 */
void lvgl_port_unlock(void);