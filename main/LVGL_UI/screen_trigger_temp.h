#pragma once

#include "lvgl.h"

/***********************
 *  GLOBAL VARIABLES
 ***********************/
extern int32_t g_trigger_temperature;  // 20-70 °C

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the trigger temperature setting screen
 * @param parent Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_trigger_temp_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_trigger_temp_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_trigger_temp_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_trigger_temp_hide(void);
