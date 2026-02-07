#include "screen_manager.h"
#include "screen_main.h"
#include "screen_brightness.h"
#include "ui_common.h"

/***********************
 *  STATIC VARIABLES
 ***********************/
static screen_id_t current_screen = SCREEN_ID_MAIN;
static lv_obj_t *screen_root = NULL;
static lv_obj_t *screen_containers[SCREEN_ID_MAX] = {NULL};
static lv_timer_t *inactivity_timer = NULL;

// Inactivity timeout in milliseconds (15 seconds)
#define INACTIVITY_TIMEOUT_MS 15000

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void inactivity_timer_cb(lv_timer_t *timer);
static void gesture_event_cb(lv_event_t *e);
static void animate_transition(lv_obj_t *out_obj, lv_obj_t *in_obj, screen_transition_t transition);
static void cleanup_screen(screen_id_t screen);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void inactivity_timer_cb(lv_timer_t *timer)
{
    // Return to main screen after inactivity
    if (current_screen != SCREEN_ID_MAIN) {
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_UP);
    }
}

static void gesture_event_cb(lv_event_t *e)
{
    screen_manager_reset_inactivity();
    
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    
    if (current_screen == SCREEN_ID_MAIN && dir == LV_DIR_BOTTOM) {
        // Swipe down from main -> brightness
        screen_manager_navigate(SCREEN_ID_BRIGHTNESS, SCREEN_TRANSITION_SLIDE_DOWN);
    } else if (current_screen == SCREEN_ID_BRIGHTNESS && dir == LV_DIR_TOP) {
        // Swipe up from brightness -> main
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_UP);
    }
}

static void animate_transition(lv_obj_t *out_obj, lv_obj_t *in_obj, screen_transition_t transition)
{
    if (transition == SCREEN_TRANSITION_NONE) {
        if (out_obj) lv_obj_add_flag(out_obj, LV_OBJ_FLAG_HIDDEN);
        if (in_obj) lv_obj_clear_flag(in_obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    lv_coord_t screen_height = lv_disp_get_ver_res(NULL);
    
    // Simple slide animation
    if (in_obj) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, in_obj);
        lv_anim_set_time(&a, 300);
        
        if (transition == SCREEN_TRANSITION_SLIDE_DOWN) {
            lv_obj_set_y(in_obj, -screen_height);
            lv_anim_set_values(&a, -screen_height, 0);
        } else if (transition == SCREEN_TRANSITION_SLIDE_UP) {
            lv_obj_set_y(in_obj, screen_height);
            lv_anim_set_values(&a, screen_height, 0);
        }
        
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_obj_clear_flag(in_obj, LV_OBJ_FLAG_HIDDEN);
        lv_anim_start(&a);
    }
    
    if (out_obj) {
        lv_obj_add_flag(out_obj, LV_OBJ_FLAG_HIDDEN);
    }
}

static void cleanup_screen(screen_id_t screen)
{
    if (screen_containers[screen]) {
        switch (screen) {
            case SCREEN_ID_MAIN:
                screen_main_destroy();
                break;
            case SCREEN_ID_BRIGHTNESS:
                screen_brightness_destroy();
                break;
            default:
                break;
        }
        screen_containers[screen] = NULL;
    }
}

void screen_manager_init(void)
{
    // Get the active screen from LVGL
    screen_root = lv_scr_act();
    lv_obj_set_style_bg_color(screen_root, COLOR_BG_PRIMARY, 0);
    
    // Start with main screen
    current_screen = SCREEN_ID_MAIN;
    screen_containers[SCREEN_ID_MAIN] = screen_main_create(screen_root);
    
    // Show the main screen
    screen_main_show();
    
    // Enable gesture navigation
    screen_manager_enable_gestures();
    
    // Start inactivity timer
    screen_manager_reset_inactivity();
}

void screen_manager_navigate(screen_id_t screen, screen_transition_t transition)
{
    if (screen >= SCREEN_ID_MAX || screen == current_screen) {
        return;
    }
    
    // Create the new screen if it doesn't exist
    if (!screen_containers[screen]) {
        switch (screen) {
            case SCREEN_ID_MAIN:
                screen_containers[screen] = screen_main_create(screen_root);
                break;
            case SCREEN_ID_BRIGHTNESS:
                screen_containers[screen] = screen_brightness_create(screen_root);
                break;
            default:
                return;
        }
    }
    
    // Animate transition
    animate_transition(screen_containers[current_screen], screen_containers[screen], transition);
    
    // Update screens
    if (screen_containers[current_screen]) {
        switch (current_screen) {
            case SCREEN_ID_MAIN:
                screen_main_hide();
                break;
            case SCREEN_ID_BRIGHTNESS:
                screen_brightness_hide();
                break;
            default:
                break;
        }
    }
    
    // Show new screen
    switch (screen) {
        case SCREEN_ID_MAIN:
            screen_main_show();
            break;
        case SCREEN_ID_BRIGHTNESS:
            screen_brightness_show();
            break;
        default:
            break;
    }
    
    current_screen = screen;
    screen_manager_reset_inactivity();
}

screen_id_t screen_manager_get_current(void)
{
    return current_screen;
}

void screen_manager_enable_gestures(void)
{
    lv_obj_add_event_cb(screen_root, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

void screen_manager_reset_inactivity(void)
{
    if (!inactivity_timer) {
        inactivity_timer = lv_timer_create(inactivity_timer_cb, INACTIVITY_TIMEOUT_MS, NULL);
    } else {
        lv_timer_reset(inactivity_timer);
    }
}

lv_obj_t *screen_manager_get_screen(void)
{
    return screen_root;
}
