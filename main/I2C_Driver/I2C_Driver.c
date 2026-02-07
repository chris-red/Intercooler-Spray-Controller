#include "I2C_Driver.h"

static const char *I2C_TAG = "I2C";

/********************* Device handle cache *********************/
#define MAX_I2C_DEVICES 8

typedef struct {
    uint8_t addr;
    i2c_master_dev_handle_t handle;
    bool used;
} i2c_device_entry_t;

i2c_master_bus_handle_t i2c_bus_handle = NULL;
static i2c_device_entry_t device_cache[MAX_I2C_DEVICES];
static int device_count = 0;

/**
 * @brief Get or create a device handle for the given I2C address
 */
static i2c_master_dev_handle_t get_or_add_device(uint8_t addr)
{
    // Check cache first
    for (int i = 0; i < device_count; i++) {
        if (device_cache[i].used && device_cache[i].addr == addr) {
            return device_cache[i].handle;
        }
    }

    // Add new device to bus
    assert(device_count < MAX_I2C_DEVICES);

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus_handle, &dev_cfg, &dev_handle));

    device_cache[device_count].addr = addr;
    device_cache[device_count].handle = dev_handle;
    device_cache[device_count].used = true;
    device_count++;

    ESP_LOGI(I2C_TAG, "Added I2C device at address 0x%02X", addr);
    return dev_handle;
}

/**
 * @brief I2C master bus initialization (new i2c_master driver)
 */
void I2C_Init(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_Touch_SCL_IO,
        .sda_io_num = I2C_Touch_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus_handle));
    ESP_LOGI(I2C_TAG, "I2C initialized successfully");
}

// Reg addr is 8 bit
esp_err_t I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
    i2c_master_dev_handle_t dev = get_or_add_device(Driver_addr);

    uint8_t buf[Length + 1];
    buf[0] = Reg_addr;
    if (Length > 0 && Reg_data != NULL) {
        memcpy(&buf[1], Reg_data, Length);
    }

    return i2c_master_transmit(dev, buf, Length + 1, I2C_MASTER_TIMEOUT_MS);
}

esp_err_t I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
    i2c_master_dev_handle_t dev = get_or_add_device(Driver_addr);

    return i2c_master_transmit_receive(dev, &Reg_addr, 1, Reg_data, Length, I2C_MASTER_TIMEOUT_MS);
}
