#pragma once

#include "lvgl.h"

/***********************
 *  GLOBAL VARIABLES
 ***********************/
extern float g_sprayer_duration;  // 0.5-10.0 seconds, 0.5 increments

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the sprayer duration setting screen
 * @param parent Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_spray_duration_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_spray_duration_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_spray_duration_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_spray_duration_hide(void);
