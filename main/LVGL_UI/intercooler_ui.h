#pragma once

#include "lvgl.h"
#include "LVGL_Driver.h"
#include "PCF85063.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "screen_main.h"
#include "screen_brightness.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize and create the intercooler UI system
 * Call this function from app_main() to set up the UI
 */
void intercooler_ui_create(void);

/**
 * Update temperature display on main screen
 * @param temp_celsius Temperature in Celsius
 */
void intercooler_ui_update_temperature(float temp_celsius);

/**
 * Update time display on main screen
 * Called automatically every second by the UI update timer
 */
void intercooler_ui_update_time(void);

/**
 * Set tank empty indicator
 * @param is_empty true if tank is empty, false otherwise
 */
void intercooler_ui_set_tank_empty(bool is_empty);

/**
 * Set relay triggered indicator
 * @param is_active true if relay is active, false otherwise
 */
void intercooler_ui_set_relay_active(bool is_active);

/**
 * Cleanup UI resources when exiting
 */
void intercooler_ui_cleanup(void);
