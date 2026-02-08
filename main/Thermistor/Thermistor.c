#include "Thermistor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

static const char *THERM_TAG = "Thermistor";

static adc_oneshot_unit_handle_t adc_handle = NULL;

/* GPIO19 = ADC2_CH8 on ESP32-S3 */
#define THERMISTOR_ADC_UNIT     ADC_UNIT_2
#define THERMISTOR_ADC_CHANNEL  ADC_CHANNEL_8
#define THERMISTOR_ADC_ATTEN    ADC_ATTEN_DB_12    /* Full 0-3.3V range */

void Thermistor_Init(void)
{
    ESP_LOGI(THERM_TAG, "Initializing thermistor on GPIO%d...", THERMISTOR_GPIO);

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = THERMISTOR_ADC_UNIT,
    };
    esp_err_t ret = adc_oneshot_new_unit(&unit_cfg, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(THERM_TAG, "adc_oneshot_new_unit failed: %s", esp_err_to_name(ret));
        return;
    }

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten = THERMISTOR_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(adc_handle, THERMISTOR_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(THERM_TAG, "adc_oneshot_config_channel failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(THERM_TAG, "Thermistor initialized on GPIO%d (ADC1_CH%d)", THERMISTOR_GPIO, THERMISTOR_ADC_CHANNEL);
}

int Thermistor_ReadRawMV(void)
{
    if (!adc_handle) return -1;

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(adc_handle, THERMISTOR_ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        ESP_LOGW(THERM_TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return -1;
    }

    /* 12-bit ADC, 3.3V range with DB_12 attenuation */
    int voltage_mv = (raw * 3300) / 4095;

    return voltage_mv;
}

float Thermistor_ReadTemp(void)
{
    /* Average multiple readings for stability */
    int sum_mv = 0;
    int valid_readings = 0;
    for (int i = 0; i < 8; i++) {
        int mv = Thermistor_ReadRawMV();
        if (mv >= 0) {
            sum_mv += mv;
            valid_readings++;
        }
    }

    if (valid_readings == 0) {
        return -999.0f;
    }

    float voltage_mv = (float)sum_mv / valid_readings;
    float voltage_v = voltage_mv / 1000.0f;

    /* Voltage divider: 3.3V --- [R_series] --- ADC --- [NTC] --- GND
     * V_adc = 3.3 * R_ntc / (R_series + R_ntc)
     * R_ntc = R_series * V_adc / (3.3 - V_adc) */
    if (voltage_v <= 0.01f || voltage_v >= 3.29f) {
        static int oor_count = 0;
        if (++oor_count >= 20) {  // Only warn every ~2 seconds
            ESP_LOGW(THERM_TAG, "Thermistor voltage out of range: %.0f mV (disconnected?)", voltage_mv);
            oor_count = 0;
        }
        return -999.0f;
    }

    float r_ntc = THERMISTOR_SERIES_R * voltage_v / (3.3f - voltage_v);

    /* Steinhart-Hart simplified (Beta equation):
     * 1/T = 1/T0 + (1/B) * ln(R/R0)
     * T in Kelvin */
    float steinhart = logf(r_ntc / THERMISTOR_NOMINAL_R) / THERMISTOR_BETA;
    steinhart += 1.0f / (THERMISTOR_NOMINAL_T + 273.15f);
    float temp_c = (1.0f / steinhart) - 273.15f;

    return temp_c;
}
