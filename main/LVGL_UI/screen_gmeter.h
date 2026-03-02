#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the G-meter screen
 * @param parent  Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_gmeter_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_gmeter_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_gmeter_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_gmeter_hide(void);
