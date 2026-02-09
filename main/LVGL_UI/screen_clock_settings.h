#pragma once

#include "lvgl.h"

/**
 * Create the clock settings screen
 * @param parent Parent object to attach the screen to
 * @return Pointer to the created screen container
 */
lv_obj_t *screen_clock_settings_create(lv_obj_t *parent);

/**
 * Destroy the clock settings screen
 */
void screen_clock_settings_destroy(void);

/**
 * Update the clock settings screen with current time from RTC
 */
void screen_clock_settings_update_ui(void);
