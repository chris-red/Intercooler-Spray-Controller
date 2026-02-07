#include "screen_trigger_temp.h"
#include "ui_common.h"
#include "screen_manager.h"

/***********************
 *  GLOBAL VARIABLES
 ***********************/
int32_t g_trigger_temperature = 40;  // Default 40°C

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container = NULL;
static lv_obj_t *slider = NULL;
static lv_obj_t *value_label = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void slider_event_cb(lv_event_t *e);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void slider_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // Reset inactivity on any interaction
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    // Update label while dragging
    if (code == LV_EVENT_PRESSING || code == LV_EVENT_RELEASED) {
        lv_obj_t *sl = lv_event_get_target(e);
        int32_t value = lv_slider_get_value(sl);

        if (value_label) {
            static int32_t last_value = -1;
            if (value != last_value) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%d°C", (int)value);
                lv_label_set_text(value_label, buf);
                last_value = value;
            }
        }

        // Commit on release
        if (code == LV_EVENT_RELEASED) {
            g_trigger_temperature = value;
        }
    }
}

lv_obj_t *screen_trigger_temp_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();

    // ===== Create container =====
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Title label
    lv_obj_t *title_label = lv_label_create(container);
    lv_label_set_text(title_label, "TRIGGER TEMP");
    lv_obj_set_style_text_color(title_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title_label, fonts->large, 0);

    // Spacer
    lv_obj_t *spacer1 = lv_obj_create(container);
    lv_obj_set_size(spacer1, 1, 30);
    lv_obj_set_style_bg_opa(spacer1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer1, 0, 0);

    // Value display label
    value_label = lv_label_create(container);
    char buf[16];
    snprintf(buf, sizeof(buf), "%d°C", (int)g_trigger_temperature);
    lv_label_set_text(value_label, buf);
    lv_obj_set_style_text_color(value_label, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(value_label, fonts->large, 0);

    // Spacer
    lv_obj_t *spacer2 = lv_obj_create(container);
    lv_obj_set_size(spacer2, 1, 30);
    lv_obj_set_style_bg_opa(spacer2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer2, 0, 0);

    // Slider: 20-70
    slider = lv_slider_create(container);
    lv_slider_set_range(slider, 20, 70);
    lv_slider_set_value(slider, g_trigger_temperature, LV_ANIM_OFF);
    lv_obj_set_size(slider, 280, 30);
    lv_obj_set_style_bg_color(slider, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_bg_color(slider, COLOR_ACCENT, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(slider, COLOR_TEXT_PRIMARY, LV_PART_KNOB);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_ALL, NULL);

    return container;
}

void screen_trigger_temp_destroy(void)
{
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    slider = NULL;
    value_label = NULL;
}

void screen_trigger_temp_show(void)
{
    // Sync slider with current value
    if (slider) {
        lv_slider_set_value(slider, g_trigger_temperature, LV_ANIM_OFF);
    }
}

void screen_trigger_temp_hide(void)
{
    // Nothing special needed on hide
}
