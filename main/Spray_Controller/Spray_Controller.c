#include "Spray_Controller.h"
#include "Buttons.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "screen_trigger_temp.h"     /* g_trigger_temperature   */
#include "screen_spray_duration.h"   /* g_sprayer_duration       */
#include "screen_spray_interval.h"   /* g_sprayer_interval       */
#include "intercooler_ui.h"          /* intercooler_ui_set_relay_active() */
#include "LVGL_Driver.h"            /* lvgl_port_lock / unlock  */

static const char *TAG = "SprayCtrl";

/***********************
 *  STATE MACHINE
 ***********************/
typedef enum {
    SPRAY_IDLE,       /**< Waiting for conditions to be met   */
    SPRAY_ACTIVE,     /**< Relay ON — spraying                */
    SPRAY_COOLDOWN,   /**< Relay OFF — waiting for interval   */
} spray_state_t;

static spray_state_t state = SPRAY_IDLE;
static int64_t       state_start_us = 0;   /* timestamp when we entered current state */
static bool          relay_is_on    = false;

/***********************
 *  HELPERS
 ***********************/
static void relay_set(bool on)
{
    if (on == relay_is_on) return;   /* avoid redundant GPIO / UI updates */
    relay_is_on = on;

    gpio_set_level(SPRAY_RELAY_GPIO, on ? 1 : 0);

    /* Update the UI spray icon */
    if (lvgl_port_lock(0)) {
        intercooler_ui_set_relay_active(on);
        lvgl_port_unlock();
    }

    ESP_LOGI(TAG, "Relay %s", on ? "ON" : "OFF");
}

/***********************
 *  PUBLIC API
 ***********************/
void Spray_Controller_Init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << SPRAY_RELAY_GPIO),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(SPRAY_RELAY_GPIO, 0);   /* default LOW */

    state       = SPRAY_IDLE;
    relay_is_on = false;
    ESP_LOGI(TAG, "Spray controller initialised (GPIO %d)", SPRAY_RELAY_GPIO);
}

void Spray_Controller_Update(float temp_celsius, bool power_on, bool tank_empty)
{
    int64_t now_us = esp_timer_get_time();

    /* ---------- Safety check (applies in ALL states) ----------
     * Power off or tank empty ⇒ kill relay immediately, reset to IDLE. */
    if (!power_on || tank_empty) {
        if (state != SPRAY_IDLE) {
            relay_set(false);
            state = SPRAY_IDLE;
            ESP_LOGI(TAG, "Abort: power=%d tank_empty=%d", power_on, tank_empty);
        }
        return;
    }

    /* ---------- State machine ---------- */
    int64_t elapsed_us = now_us - state_start_us;

    switch (state) {

    case SPRAY_IDLE:
        /* Only start a new spray when we have a valid reading above trigger.
         * Invalid / below-trigger readings are simply ignored (stay IDLE). */
        if (temp_celsius > -900.0f &&
            temp_celsius > (float)g_trigger_temperature) {
            relay_set(true);
            state = SPRAY_ACTIVE;
            state_start_us = now_us;
            ESP_LOGI(TAG, "EVENT: Spray START  duration=%.1fs  interval=%lds  temp=%.1f°C  trigger=%ld°C",
                     g_sprayer_duration, (long)g_sprayer_interval,
                     temp_celsius, (long)g_trigger_temperature);
        }
        break;

    case SPRAY_ACTIVE: {
        /* Run for the full spray duration regardless of temp fluctuations.
         * Only power-off / tank-empty (handled above) can cancel early. */
        int64_t duration_us = (int64_t)(g_sprayer_duration * 1000000.0f);
        if (elapsed_us >= duration_us) {
            relay_set(false);
            state = SPRAY_COOLDOWN;
            state_start_us = now_us;
            ESP_LOGI(TAG, "EVENT: Spray END  cooldown=%lds", (long)g_sprayer_interval);
        }
        break;
    }

    case SPRAY_COOLDOWN: {
        /* Wait the full interval before allowing another spray. */
        int64_t interval_us = (int64_t)g_sprayer_interval * 1000000LL;
        if (elapsed_us >= interval_us) {
            state = SPRAY_IDLE;
            ESP_LOGI(TAG, "Cooldown done, ready to spray again");
        }
        break;
    }

    default:
        state = SPRAY_IDLE;
        break;
    }
}

bool Spray_Controller_IsSpraying(void)
{
    return (state == SPRAY_ACTIVE);
}

bool Spray_Controller_IsCooldown(void)
{
    return (state == SPRAY_COOLDOWN);
}
