#pragma once

#include "lvgl.h"

/***********************
 *  GLOBAL VARIABLES
 ***********************/
extern int32_t g_sprayer_interval;  // 5-30 seconds, 1 second increments

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the sprayer interval setting screen
 * @param parent Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_spray_interval_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_spray_interval_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_spray_interval_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_spray_interval_hide(void);
