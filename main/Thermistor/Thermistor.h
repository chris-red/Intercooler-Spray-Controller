#pragma once

#include <stdbool.h>

/***********************
 *  PIN CONFIGURATION
 ***********************/
#define THERMISTOR_GPIO         19      // GPIO19 = ADC2_CH8

/***********************
 *  THERMISTOR PARAMETERS
 *  100K NTC with B=3950
 *  Using voltage divider: 3.3V --- [100K fixed] --- ADC_PIN --- [NTC] --- GND
 ***********************/
#define THERMISTOR_NOMINAL_R    100000.0f   // Resistance at 25°C (ohms)
#define THERMISTOR_NOMINAL_T    25.0f       // Nominal temperature (°C)
#define THERMISTOR_BETA         3950.0f     // Beta coefficient
#define THERMISTOR_SERIES_R     100000.0f   // Series resistor value (ohms)

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize the thermistor ADC input
 */
void Thermistor_Init(void);

/**
 * Read the current temperature in Celsius
 * @return Temperature in °C, or -999.0f on error
 */
float Thermistor_ReadTemp(void);

/**
 * Read raw ADC voltage in millivolts (for debugging)
 * @return Voltage in mV
 */
int Thermistor_ReadRawMV(void);
