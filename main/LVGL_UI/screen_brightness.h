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

/***********************
 *  GLOBAL VARIABLES
 ***********************/
extern uint8_t g_brightness;

/**
 * Set the LCD brightness and update g_brightness
 * @param brightness Brightness value 0-100
 */
void set_brightness(uint8_t brightness);

/**
 * Update the brightness slider and label to match g_brightness
 */
void screen_brightness_update_ui(void);

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
