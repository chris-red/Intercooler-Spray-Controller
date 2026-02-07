#include "intercooler_ui.h"

void intercooler_ui_create(void)
{
    // Initialize fonts
    ui_fonts_t fonts;
    ui_common_init_fonts(&fonts);
    
    // Initialize screen manager (creates main screen)
    screen_manager_init();
}

void intercooler_ui_update_temperature(float temp_celsius)
{
    screen_main_update_temperature(temp_celsius);
}

void intercooler_ui_update_time(void)
{
    screen_main_update_time();
}

void intercooler_ui_set_tank_empty(bool is_empty)
{
    screen_main_set_tank_empty(is_empty);
}

void intercooler_ui_set_relay_active(bool is_active)
{
    screen_main_set_relay_active(is_active);
}

void intercooler_ui_set_power_on(bool is_on)
{
    screen_main_set_power_on(is_on);
}

void intercooler_ui_cleanup(void)
{
    // Cleanup handled by screen manager and individual screens
}
