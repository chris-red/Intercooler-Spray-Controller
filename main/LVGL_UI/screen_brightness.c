#include "screen_brightness.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "driver/ledc.h"
#include "driver/gpio.h"

/***********************
 *  DEFINES
 ***********************/
#define BRIGHTNESS_GPIO     GPIO_NUM_6
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL        LEDC_CHANNEL_0
#define LEDC_DUTY_RES       LEDC_TIMER_8_BIT  // 8-bit resolution (0-255)
#define LEDC_FREQUENCY      5000              // 5 kHz PWM frequency

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container = NULL;
static lv_obj_t *brightness_slider = NULL;
static lv_obj_t *brightness_value_label = NULL;
static uint8_t current_brightness = 128;  // Default 50%
static bool pwm_initialized = false;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void brightness_slider_event_cb(lv_event_t *e);
static void init_pwm(void);
static void set_brightness_pwm(uint8_t brightness);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void init_pwm(void)
{
    if (pwm_initialized) return;
    
    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .timer_num        = LEDC_TIMER,
        .duty_resolution  = LEDC_DUTY_RES,
        .freq_hz          = LEDC_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .timer_sel      = LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BRIGHTNESS_GPIO,
        .duty           = current_brightness,
        .hpoint         = 0
    };
    ledc_channel_config(&ledc_channel);
    
    pwm_initialized = true;
}

static void set_brightness_pwm(uint8_t brightness)
{
    if (!pwm_initialized) {
        init_pwm();
    }
    
    current_brightness = brightness;
    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, brightness);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

static void brightness_slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    // Reset inactivity on any interaction
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }
    
    // Update label while dragging
    if (code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED) {
        lv_obj_t *slider = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(slider);
        
        if (brightness_value_label) {
            static int32_t last_value = -1;
            // Only update if value actually changed to reduce redraws
            if (value != last_value) {
                char buf[16];
                int percent = (value * 100) / 255;
                snprintf(buf, sizeof(buf), "%d%%", percent);
                lv_label_set_text(brightness_value_label, buf);
                last_value = value;
            }
        }
        
        // Update PWM only on release
        if (code == LV_EVENT_RELEASED) {
            set_brightness_pwm((uint8_t)value);
        }
    }
}

lv_obj_t *screen_brightness_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();
    
    // Initialize PWM for brightness control
    init_pwm();
    
    // ===== Create container =====
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling - no scrollable content
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);  // Allow gestures to bubble up

    // Title label
    lv_obj_t *title_label = lv_label_create(container);
    lv_label_set_text(title_label, "BRIGHTNESS");
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_label, fonts->large, 0);

    // Spacer
    lv_obj_t *spacer1 = lv_obj_create(container);
    lv_obj_set_size(spacer1, 1, 30);
    lv_obj_set_style_bg_opa(spacer1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer1, 0, 0);

    // Value display label
    brightness_value_label = lv_label_create(container);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", (int)((current_brightness * 100) / 255));
    lv_label_set_text(brightness_value_label, buf);
    lv_obj_set_style_text_color(brightness_value_label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(brightness_value_label, fonts->large, 0);

    // Spacer
    lv_obj_t *spacer2 = lv_obj_create(container);
    lv_obj_set_size(spacer2, 1, 30);
    lv_obj_set_style_bg_opa(spacer2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer2, 0, 0);

    // Brightness slider
    brightness_slider = lv_slider_create(container);
    lv_slider_set_range(brightness_slider, 0, 255);
    lv_slider_set_value(brightness_slider, current_brightness, LV_ANIM_OFF);
    lv_obj_set_size(brightness_slider, 280, 30);
    lv_obj_set_style_bg_color(brightness_slider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(brightness_slider, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(brightness_slider, COLOR_TEXT_PRIMARY, LV_PART_KNOB);
    lv_obj_add_event_cb(brightness_slider, brightness_slider_event_cb, LV_EVENT_ALL, NULL);

    // Instructions label
    lv_obj_t *instr_label = lv_label_create(container);
    lv_label_set_text(instr_label, "Swipe up to return");
    lv_obj_set_style_text_color(instr_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(instr_label, fonts->small, 0);
    lv_obj_set_style_pad_top(instr_label, 40, 0);
    
    return container;
}

void screen_brightness_destroy(void)
{
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    
    brightness_slider = NULL;
    brightness_value_label = NULL;
}

void screen_brightness_show(void)
{
    // Nothing special needed on show
}

void screen_brightness_hide(void)
{
    // Nothing special needed on hide
}
