#include "screen_manager.h"
#include "screen_main.h"
#include "screen_brightness.h"
#include "screen_trigger_temp.h"
#include "screen_spray_duration.h"
#include "screen_spray_interval.h"
#include "ui_common.h"
#include "esp_log.h"

static const char *TAG = "screen_mgr";

/***********************
 *  STATIC VARIABLES
 ***********************/
static screen_id_t current_screen = SCREEN_ID_MAIN;
static lv_obj_t *screen_root = NULL;
static lv_obj_t *screen_containers[SCREEN_ID_MAX] = {NULL};
static lv_timer_t *inactivity_timer = NULL;

// Inactivity timeout in milliseconds (15 seconds)
#define INACTIVITY_TIMEOUT_MS 15000

// Minimum time between screen transitions (ms) to prevent accidental double-swipe
#define NAV_COOLDOWN_MS 400

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void inactivity_timer_cb(lv_timer_t *timer);
static void gesture_event_cb(lv_event_t *e);
static void animate_transition(lv_obj_t *out_obj, lv_obj_t *in_obj, screen_transition_t transition);
static void cleanup_screen(screen_id_t screen);
static bool nav_cooldown_check(void);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static uint32_t last_nav_time = 0;

static bool nav_cooldown_check(void)
{
    uint32_t now = lv_tick_get();
    if (now - last_nav_time < NAV_COOLDOWN_MS) {
        return false;  // Still in cooldown
    }
    last_nav_time = now;
    return true;
}

static void inactivity_timer_cb(lv_timer_t *timer)
{
    // Return to main screen after inactivity with correct transition direction
    if (current_screen == SCREEN_ID_BRIGHTNESS) {
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_UP);
    } else if (current_screen != SCREEN_ID_MAIN) {
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_RIGHT);
    }
}

static void gesture_event_cb(lv_event_t *e)
{
    // Cooldown guard to prevent accidental rapid transitions
    if (!nav_cooldown_check()) {
        ESP_LOGI(TAG, "Gesture IGNORED - cooldown active");
        return;
    }

    screen_manager_reset_inactivity();
    
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    const char *dir_str = (dir == LV_DIR_LEFT) ? "LEFT" :
                          (dir == LV_DIR_RIGHT) ? "RIGHT" :
                          (dir == LV_DIR_TOP) ? "UP" :
                          (dir == LV_DIR_BOTTOM) ? "DOWN" : "UNKNOWN";
    ESP_LOGI(TAG, "Swipe %s on screen %d", dir_str, current_screen);
    
    if (current_screen == SCREEN_ID_MAIN && dir == LV_DIR_BOTTOM) {
        ESP_LOGI(TAG, "NAV: main -> brightness");
        screen_manager_navigate(SCREEN_ID_BRIGHTNESS, SCREEN_TRANSITION_SLIDE_DOWN);
    } else if (current_screen == SCREEN_ID_BRIGHTNESS && dir == LV_DIR_TOP) {
        ESP_LOGI(TAG, "NAV: brightness -> main");
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_UP);
    } else if (current_screen == SCREEN_ID_MAIN && dir == LV_DIR_LEFT) {
        ESP_LOGI(TAG, "NAV: main -> trigger temp");
        screen_manager_navigate(SCREEN_ID_TRIGGER_TEMP, SCREEN_TRANSITION_SLIDE_LEFT);
    } else if (current_screen == SCREEN_ID_TRIGGER_TEMP && dir == LV_DIR_RIGHT) {
        ESP_LOGI(TAG, "NAV: trigger temp -> main");
        screen_manager_navigate(SCREEN_ID_MAIN, SCREEN_TRANSITION_SLIDE_RIGHT);
    } else if (current_screen == SCREEN_ID_TRIGGER_TEMP && dir == LV_DIR_LEFT) {
        ESP_LOGI(TAG, "NAV: trigger temp -> spray duration");
        screen_manager_navigate(SCREEN_ID_SPRAY_DURATION, SCREEN_TRANSITION_SLIDE_LEFT);
    } else if (current_screen == SCREEN_ID_SPRAY_DURATION && dir == LV_DIR_RIGHT) {
        ESP_LOGI(TAG, "NAV: spray duration -> trigger temp");
        screen_manager_navigate(SCREEN_ID_TRIGGER_TEMP, SCREEN_TRANSITION_SLIDE_RIGHT);
    } else if (current_screen == SCREEN_ID_SPRAY_DURATION && dir == LV_DIR_LEFT) {
        ESP_LOGI(TAG, "NAV: spray duration -> spray interval");
        screen_manager_navigate(SCREEN_ID_SPRAY_INTERVAL, SCREEN_TRANSITION_SLIDE_LEFT);
    } else if (current_screen == SCREEN_ID_SPRAY_INTERVAL && dir == LV_DIR_RIGHT) {
        ESP_LOGI(TAG, "NAV: spray interval -> spray duration");
        screen_manager_navigate(SCREEN_ID_SPRAY_DURATION, SCREEN_TRANSITION_SLIDE_RIGHT);
    } else {
        ESP_LOGW(TAG, "Swipe %s on screen %d - NO MATCHING ROUTE", dir_str, current_screen);
    }
}

static void animate_transition(lv_obj_t *out_obj, lv_obj_t *in_obj, screen_transition_t transition)
{
    ESP_LOGI(TAG, "animate_transition: out=%p in=%p transition=%d", (void*)out_obj, (void*)in_obj, transition);
    if (transition == SCREEN_TRANSITION_NONE) {
        if (out_obj) lv_obj_add_flag(out_obj, LV_OBJ_FLAG_HIDDEN);
        if (in_obj) lv_obj_clear_flag(in_obj, LV_OBJ_FLAG_HIDDEN);
        return;
    }
    
    lv_coord_t screen_height = lv_disp_get_ver_res(NULL);
    lv_coord_t screen_width = lv_disp_get_hor_res(NULL);
    
    // Simple slide animation
    if (in_obj) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, in_obj);
        lv_anim_set_time(&a, 300);
        
        if (transition == SCREEN_TRANSITION_SLIDE_DOWN) {
            lv_obj_set_y(in_obj, -screen_height);
            lv_anim_set_values(&a, -screen_height, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        } else if (transition == SCREEN_TRANSITION_SLIDE_UP) {
            lv_obj_set_y(in_obj, screen_height);
            lv_anim_set_values(&a, screen_height, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        } else if (transition == SCREEN_TRANSITION_SLIDE_LEFT) {
            lv_obj_set_x(in_obj, screen_width);
            lv_anim_set_values(&a, screen_width, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
        } else if (transition == SCREEN_TRANSITION_SLIDE_RIGHT) {
            lv_obj_set_x(in_obj, -screen_width);
            lv_anim_set_values(&a, -screen_width, 0);
            lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_x);
        }
        
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
            case SCREEN_ID_TRIGGER_TEMP:
                screen_trigger_temp_destroy();
                break;
            case SCREEN_ID_SPRAY_DURATION:
                screen_spray_duration_destroy();
                break;
            case SCREEN_ID_SPRAY_INTERVAL:
                screen_spray_interval_destroy();
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
    
    // Increase gesture detection threshold for more reliable swipes
    lv_indev_t *indev = NULL;
    while ((indev = lv_indev_get_next(indev)) != NULL) {
        if (lv_indev_get_type(indev) == LV_INDEV_TYPE_POINTER) {
            indev->driver->gesture_limit = 80;       // Require longer swipe (default 50)
            indev->driver->gesture_min_velocity = 4;  // Slightly higher velocity threshold
            break;
        }
    }
    
    // Start inactivity timer
    screen_manager_reset_inactivity();
}

void screen_manager_navigate(screen_id_t screen, screen_transition_t transition)
{
    ESP_LOGI(TAG, "navigate() called: from screen %d to screen %d, transition=%d", current_screen, screen, transition);

    if (screen >= SCREEN_ID_MAX || screen == current_screen) {
        ESP_LOGW(TAG, "navigate() ABORTED: invalid screen %d or same as current %d", screen, current_screen);
        return;
    }
    
    // Create the new screen if it doesn't exist
    if (!screen_containers[screen]) {
        ESP_LOGI(TAG, "Creating screen %d (container was NULL)", screen);
        switch (screen) {
            case SCREEN_ID_MAIN:
                screen_containers[screen] = screen_main_create(screen_root);
                break;
            case SCREEN_ID_BRIGHTNESS:
                screen_containers[screen] = screen_brightness_create(screen_root);
                break;
            case SCREEN_ID_TRIGGER_TEMP:
                screen_containers[screen] = screen_trigger_temp_create(screen_root);
                break;
            case SCREEN_ID_SPRAY_DURATION:
                screen_containers[screen] = screen_spray_duration_create(screen_root);
                break;
            case SCREEN_ID_SPRAY_INTERVAL:
                screen_containers[screen] = screen_spray_interval_create(screen_root);
                break;
            default:
                ESP_LOGE(TAG, "navigate() FAILED: unknown screen %d", screen);
                return;
        }
        ESP_LOGI(TAG, "Screen %d created, container=%p", screen, (void*)screen_containers[screen]);
    } else {
        ESP_LOGI(TAG, "Screen %d already exists, container=%p", screen, (void*)screen_containers[screen]);
    }
    
    // Animate transition
    ESP_LOGI(TAG, "Animating transition: out=%p in=%p", (void*)screen_containers[current_screen], (void*)screen_containers[screen]);
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
            case SCREEN_ID_TRIGGER_TEMP:
                screen_trigger_temp_hide();
                break;
            case SCREEN_ID_SPRAY_DURATION:
                screen_spray_duration_hide();
                break;
            case SCREEN_ID_SPRAY_INTERVAL:
                screen_spray_interval_hide();
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
        case SCREEN_ID_TRIGGER_TEMP:
            screen_trigger_temp_show();
            break;
        case SCREEN_ID_SPRAY_DURATION:
            screen_spray_duration_show();
            break;
        case SCREEN_ID_SPRAY_INTERVAL:
            screen_spray_interval_show();
            break;
        default:
            break;
    }
    
    current_screen = screen;
    ESP_LOGI(TAG, "Navigation complete. current_screen is now %d", current_screen);
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
