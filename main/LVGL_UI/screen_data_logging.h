#pragma once

#include "lvgl.h"
#include <stdbool.h>

/***********************
 *  GLOBAL VARIABLES
 ***********************/
extern bool g_logging_enabled;  // Persisted on/off state

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Create the data-logging toggle screen
 * @param parent  Parent object to attach to
 * @return Container object for the screen
 */
lv_obj_t *screen_data_logging_create(lv_obj_t *parent);

/**
 * Destroy the screen and free resources
 */
void screen_data_logging_destroy(void);

/**
 * Called when the screen becomes visible
 */
void screen_data_logging_show(void);

/**
 * Called when the screen becomes hidden
 */
void screen_data_logging_hide(void);
