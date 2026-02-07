#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the main screen
 * @param parent Parent object to attach to
 * @return Container object for the main screen
 */
lv_obj_t *screen_main_create(lv_obj_t *parent);

/**
 * Destroy the main screen and free resources
 */
void screen_main_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_main_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_main_hide(void);

/**
 * Update temperature display
 * @param temp_celsius Temperature in Celsius
 */
void screen_main_update_temperature(float temp_celsius);

/**
 * Update time display
 */
void screen_main_update_time(void);

/**
 * Set tank empty indicator
 * @param is_empty True if tank is empty
 */
void screen_main_set_tank_empty(bool is_empty);

/**
 * Set relay active indicator
 * @param is_active True if relay is active
 */
void screen_main_set_relay_active(bool is_active);

/**
 * Set power on indicator
 * @param is_on True if system is powered on
 */
void screen_main_set_power_on(bool is_on);
