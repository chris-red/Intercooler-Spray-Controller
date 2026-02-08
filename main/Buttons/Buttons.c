#include "Buttons.h"

#if ENABLE_BUTTONS

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"

static const char *BTN_TAG = "Buttons";

/***********************
 *  INTERNAL STATE
 ***********************/
static volatile bool power_raw_pressed = false;
static volatile bool tank_raw_pressed = false;

static TimerHandle_t power_debounce_timer = NULL;
static TimerHandle_t tank_debounce_timer = NULL;

#define DEBOUNCE_MS  50

/***********************
 *  DEBOUNCE CALLBACKS
 ***********************/
static void power_debounce_cb(TimerHandle_t timer)
{
    power_raw_pressed = (gpio_get_level(BUTTON_POWER_GPIO) == 0);
}

static void tank_debounce_cb(TimerHandle_t timer)
{
    tank_raw_pressed = (gpio_get_level(BUTTON_TANK_GPIO) == 0);
}

/***********************
 *  ISR HANDLERS
 ***********************/
static void IRAM_ATTR power_isr_handler(void *arg)
{
    BaseType_t wake = pdFALSE;
    xTimerResetFromISR(power_debounce_timer, &wake);
    if (wake) portYIELD_FROM_ISR();
}

static void IRAM_ATTR tank_isr_handler(void *arg)
{
    BaseType_t wake = pdFALSE;
    xTimerResetFromISR(tank_debounce_timer, &wake);
    if (wake) portYIELD_FROM_ISR();
}

/***********************
 *  PUBLIC FUNCTIONS
 ***********************/
void Buttons_Init(void)
{
    power_debounce_timer = xTimerCreate("pwr_dbnc", pdMS_TO_TICKS(DEBOUNCE_MS),
                                         pdFALSE, NULL, power_debounce_cb);
    tank_debounce_timer  = xTimerCreate("tnk_dbnc", pdMS_TO_TICKS(DEBOUNCE_MS),
                                         pdFALSE, NULL, tank_debounce_cb);

    gpio_config_t btn_cfg = {
        .pin_bit_mask = (1ULL << BUTTON_POWER_GPIO) | (1ULL << BUTTON_TANK_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    ESP_ERROR_CHECK(gpio_config(&btn_cfg));

    esp_err_t isr_ret = gpio_install_isr_service(0);
    if (isr_ret != ESP_OK && isr_ret != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(isr_ret);
    }
    gpio_isr_handler_add(BUTTON_POWER_GPIO, power_isr_handler, NULL);
    gpio_isr_handler_add(BUTTON_TANK_GPIO, tank_isr_handler, NULL);

    ESP_LOGI(BTN_TAG, "Buttons initialized: Power=GPIO%d, Tank=GPIO%d",
             BUTTON_POWER_GPIO, BUTTON_TANK_GPIO);
}

bool Button_Power_GetState(void) { return power_raw_pressed; }
bool Button_Tank_GetState(void)  { return tank_raw_pressed; }

#endif /* ENABLE_BUTTONS */
