#include "ui_common.h"

static ui_fonts_t g_fonts = {0};

void ui_common_init_fonts(ui_fonts_t *fonts)
{
    fonts->temp = &race_120;
    fonts->large = &lv_font_montserrat_48;
    fonts->normal = &lv_font_montserrat_12;
    fonts->small = &lv_font_montserrat_12;
    
    // Store globally for easy access
    g_fonts = *fonts;
}

const ui_fonts_t *ui_common_get_fonts(void)
{
    return &g_fonts;
}

void ui_common_set_label_color(lv_obj_t *label, const char *text, lv_color_t color)
{
    if (label != NULL) {
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, color, 0);
    }
}
