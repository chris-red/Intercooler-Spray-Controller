#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the brightness control screen
 * @param parent Parent object to attach to
 * @return Container object for the brightness screen
 */
lv_obj_t *screen_brightness_create(lv_obj_t *parent);

/**
 * Destroy the brightness screen and free resources
 */
void screen_brightness_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_brightness_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_brightness_hide(void);
