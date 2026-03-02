#include "Thermistor.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include <math.h>

static const char *THERM_TAG = "Thermistor";

/* GM IAT Sensor resistance vs temperature lookup table
 * Calibrated values based on actual sensor measurements */
typedef struct {
    float temp_c;
    float resistance_ohms;
} iat_lookup_t;

static const iat_lookup_t iat_table[] = {
    {-40.0f, 100700.0f},
    {-30.0f, 52700.0f},
    {-20.0f, 28680.0f},
    {-10.0f, 16180.0f},
    {  0.0f, 9420.0f},
    {  5.0f, 6800.0f},
    { 10.0f, 5000.0f},
    { 15.0f, 3700.0f},
    { 20.0f, 2800.0f},
    { 22.5f, 2550.0f},  // Measured calibration point
    { 25.0f, 2350.0f},
    { 30.0f, 1860.0f},
    { 40.0f, 1150.0f},
    { 50.0f, 750.0f},
    { 60.0f, 500.0f},
    { 70.0f, 340.0f},
    { 80.0f, 240.0f},
    { 90.0f, 175.0f},
    {100.0f, 130.0f},
    {110.0f, 100.0f},
    {120.0f, 77.0f}
};

#define IAT_TABLE_SIZE (sizeof(iat_table) / sizeof(iat_table[0]))

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

    /* Voltage divider: 3.3V --- [R_series] --- ADC --- [IAT Sensor] --- GND
     * V_adc = 3.3 * R_iat / (R_series + R_iat)
     * R_iat = R_series * V_adc / (3.3 - V_adc) */
    if (voltage_v <= 0.01f || voltage_v >= 3.29f) {
        static int oor_count = 0;
        if (++oor_count >= 20) {  // Only warn every ~2 seconds
            ESP_LOGW(THERM_TAG, "IAT sensor voltage out of range: %.0f mV (disconnected?)", voltage_mv);
            oor_count = 0;
        }
        return -999.0f;
    }

    float r_iat = THERMISTOR_SERIES_R * voltage_v / (3.3f - voltage_v);

    /* Debug logging - log every 50 calls (about every 5 seconds) */
    static int log_counter = 0;
    if (++log_counter >= 50) {
        ESP_LOGI(THERM_TAG, "ADC: %.0f mV, R_IAT: %.0f Ω", voltage_mv, r_iat);
        log_counter = 0;
    }

    /* Convert resistance to temperature using lookup table with linear interpolation */
    /* IAT sensors have resistance that decreases with temperature (NTC) */
    
    /* Check if resistance is out of table bounds */
    if (r_iat >= iat_table[0].resistance_ohms) {
        /* Colder than minimum table entry */
        return iat_table[0].temp_c;
    }
    if (r_iat <= iat_table[IAT_TABLE_SIZE - 1].resistance_ohms) {
        /* Hotter than maximum table entry */
        return iat_table[IAT_TABLE_SIZE - 1].temp_c;
    }

    /* Find the two table entries that bracket our resistance */
    for (int i = 0; i < IAT_TABLE_SIZE - 1; i++) {
        if (r_iat <= iat_table[i].resistance_ohms && r_iat >= iat_table[i + 1].resistance_ohms) {
            /* Linear interpolation between iat_table[i] and iat_table[i+1] */
            float r_low = iat_table[i + 1].resistance_ohms;
            float r_high = iat_table[i].resistance_ohms;
            float t_low = iat_table[i + 1].temp_c;
            float t_high = iat_table[i].temp_c;
            
            /* Interpolate */
            float temp_c = t_high + (t_low - t_high) * (r_iat - r_high) / (r_low - r_high);
            
            return temp_c;
        }
    }

    /* Should never reach here */
    return -999.0f;
}
