#pragma once

#include "lvgl.h"

/***********************
 *  TYPE DEFINITIONS
 ***********************/
typedef enum {
    SCREEN_ID_MAIN,
    SCREEN_ID_BRIGHTNESS,
    SCREEN_ID_MAX
} screen_id_t;

typedef enum {
    SCREEN_TRANSITION_NONE,
    SCREEN_TRANSITION_SLIDE_UP,
    SCREEN_TRANSITION_SLIDE_DOWN,
    SCREEN_TRANSITION_SLIDE_LEFT,
    SCREEN_TRANSITION_SLIDE_RIGHT
} screen_transition_t;

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize the screen manager
 */
void screen_manager_init(void);

/**
 * Navigate to a specific screen with optional transition
 * @param screen Screen ID to navigate to
 * @param transition Transition animation type
 */
void screen_manager_navigate(screen_id_t screen, screen_transition_t transition);

/**
 * Get the current active screen ID
 */
screen_id_t screen_manager_get_current(void);

/**
 * Setup gesture navigation (swipe between screens)
 */
void screen_manager_enable_gestures(void);

/**
 * Reset inactivity timer (call on user interaction)
 */
void screen_manager_reset_inactivity(void);

/**
 * Get the root screen object
 */
lv_obj_t *screen_manager_get_screen(void);
