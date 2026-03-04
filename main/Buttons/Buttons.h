#pragma once

#include <stdbool.h>

/***********************
 *  ENABLE/DISABLE BUTTONS
 *  Buttons share GPIO43/44 with UART console. To switch modes:
 *
 *  BUTTONS ON  (production):  Set ENABLE_BUTTONS=1 below
 *              + sdkconfig:   CONFIG_ESP_CONSOLE_NONE=y
 *
 *  BUTTONS OFF (debugging):   Set ENABLE_BUTTONS=0 below
 *              + sdkconfig:   CONFIG_ESP_CONSOLE_UART_DEFAULT=y
 *
 *  NOTE: USB logging is unavailable either way (GPIO19 = thermistor)
 ***********************/
#define ENABLE_BUTTONS   1

/***********************
 *  PIN CONFIGURATION
 ***********************/
#define BUTTON_POWER_GPIO       43      // GPIO43 (UART TXD) — Power on/off button
#define BUTTON_TANK_GPIO        44      // GPIO44 (UART RXD) — Tank empty indicator

/***********************
 *  BUTTON CONFIGURATION
 *  Buttons should be wired: GPIO --- [Button] --- GND
 *  Internal pull-ups are enabled, so buttons are active LOW.
 *  Software debounce is applied (50ms).
 ***********************/

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

#if ENABLE_BUTTONS

/** Initialize button GPIO inputs with internal pull-ups and ISR debounce */
void Buttons_Init(void);

/** @return true if power button is currently held */
bool Button_Power_GetState(void);

/** @return true if tank empty button is currently held */
bool Button_Tank_GetState(void);

#else

/* Stubs when buttons are disabled — compiles to nothing */
static inline void Buttons_Init(void) {}
static inline bool Button_Power_GetState(void) { return false; }
static inline bool Button_Tank_GetState(void)  { return false; }

#endif
