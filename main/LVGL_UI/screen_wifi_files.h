#pragma once

#include "lvgl.h"

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

lv_obj_t *screen_wifi_files_create(lv_obj_t *parent);
void screen_wifi_files_destroy(void);
void screen_wifi_files_show(void);
void screen_wifi_files_hide(void);
