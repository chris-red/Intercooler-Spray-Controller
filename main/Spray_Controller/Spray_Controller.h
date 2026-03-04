#pragma once

#include <stdbool.h>

/***********************
 *  RELAY PIN
 ***********************/
#define SPRAY_RELAY_GPIO    20

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize the spray relay GPIO (output, default LOW).
 * Must be called once during startup.
 */
void Spray_Controller_Init(void);

/**
 * Call periodically from the driver loop (every 100 ms).
 *
 * Logic:
 *   - Power must be ON, tank must NOT be empty, temp must exceed trigger temp
 *   - Activates relay for the configured spray duration
 *   - After spraying, waits the configured spray interval before allowing
 *     another spray cycle
 *
 * @param temp_celsius  Current temperature reading (ignored if < -900)
 * @param power_on      true when the power button is engaged
 * @param tank_empty    true when the tank-empty sensor is active
 */
void Spray_Controller_Update(float temp_celsius, bool power_on, bool tank_empty);

/** @return true while the relay is ON (spraying) */
bool Spray_Controller_IsSpraying(void);

/** @return true while in the cooldown interval after a spray */
bool Spray_Controller_IsCooldown(void);
