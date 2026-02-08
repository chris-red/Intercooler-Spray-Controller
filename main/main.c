#include <stdio.h>
//#include "sdkconfig.h"
//#include "freertos/FreeRTOS.h"
//#include "freertos/task.h"
//#include "freertos/semphr.h"
//#include "TCA9554PWR.h"
//#include "PCF85063.h"
//#include "QMI8658.h"
#include "ST7701S.h"
////#include "CST820.h"
#include "SD_MMC.h"
#include "LVGL_Driver.h"
#include "LVGL_Example.h"
#include "intercooler_ui.h"
//#include "Wireless.h"
#include "lvgl.h"
#include "Thermistor.h"
#include "Buttons.h"


// Redraw the entire screen by clearing and recreating the UI
void refresh_screen(void) {
    lvgl_port_lock(0);
    lv_obj_clean(lv_scr_act());
    intercooler_ui_create();
    lv_refr_now(NULL);
    lvgl_port_unlock();
}

void Driver_Loop(void *parameter)
{
    static int therm_log_counter = 0;
#if ENABLE_BUTTONS
    static bool prev_power_state = false;
    static bool prev_tank_state = false;
#endif

    while(1)
    {
       // QMI8658_Loop();
        RTC_Loop();
       // BAT_Get_Volts();

        // --- Read thermistor and update UI ---
        float temp = Thermistor_ReadTemp();
        int raw_mv = Thermistor_ReadRawMV();
        if (++therm_log_counter >= 20) {  // Log every 2 seconds (20 x 100ms)
            printf("Thermistor: %d mV, %.1f°C\n", raw_mv, temp);
            therm_log_counter = 0;
        }
        if (temp > -900.0f) {  // Valid reading
            if (lvgl_port_lock(0)) {
                intercooler_ui_update_temperature(temp);
                lvgl_port_unlock();
            }
        }

#if ENABLE_BUTTONS
        // --- Read buttons and update UI ---
        bool power_on = Button_Power_GetState();
        if (power_on != prev_power_state) {
            if (lvgl_port_lock(0)) {
                intercooler_ui_set_power_on(power_on);
                lvgl_port_unlock();
            }
            prev_power_state = power_on;
        }

        bool tank_empty = Button_Tank_GetState();
        if (tank_empty != prev_tank_state) {
            if (lvgl_port_lock(0)) {
                intercooler_ui_set_tank_empty(tank_empty);
                lvgl_port_unlock();
            }
            prev_tank_state = tank_empty;
        }
#endif

        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);
}
void Driver_Init(void)
{
    Flash_Searching();
    //BAT_Init();
    I2C_Init();
    PCF85063_Init();
   // QMI8658_Init();
    EXIO_Init();                    // Example Initialize EXIO
    printf("EXIO init done, starting Thermistor...\n");
    Thermistor_Init();               // Initialize thermistor ADC on GPIO19
    printf("Thermistor init done\n");
#if ENABLE_BUTTONS
    Buttons_Init();                  // Initialize buttons on GPIO43/44 (disables UART!)
    printf("Buttons init done\n");
#endif
    xTaskCreatePinnedToCore(
        Driver_Loop, 
        "Other Driver task",
        4096, 
        NULL, 
        3, 
        NULL, 
        0);
}
void app_main(void)
{   
    // Allow hardware (I2C expander, LCD) to stabilize after power-on/reset
    vTaskDelay(pdMS_TO_TICKS(1000));

  //  Wireless_Init();
    Driver_Init();
    LCD_Init();
    Touch_Init();
    SD_Init();
    LVGL_Init();
    printf("init complete\n");
/********************* Intercooler UI *********************/
    lvgl_port_lock(0);
    intercooler_ui_create();
    lvgl_port_unlock();
    printf("UI created\n");

    while (1) {
        // raise the task priority of LVGL and/or reduce the handler period can improve the performance
        vTaskDelay(pdMS_TO_TICKS(10));
        // The task running lv_timer_handler should have lower priority than that running `lv_tick_inc`
        lvgl_port_lock(0);
        lv_timer_handler();
        lvgl_port_unlock();
    }
}
