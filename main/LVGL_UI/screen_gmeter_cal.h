#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the G-meter calibration screen.
 * @param parent  Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_gmeter_cal_create(lv_obj_t *parent);

/**
 * Destroy the calibration screen and free resources.
 */
void screen_gmeter_cal_destroy(void);

/**
 * Called when the screen becomes visible.
 */
void screen_gmeter_cal_show(void);

/**
 * Called when the screen is hidden.
 */
void screen_gmeter_cal_hide(void);
