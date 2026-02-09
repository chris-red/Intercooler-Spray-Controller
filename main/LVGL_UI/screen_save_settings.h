#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the save settings screen
 * @param parent Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_save_settings_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_save_settings_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_save_settings_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_save_settings_hide(void);
