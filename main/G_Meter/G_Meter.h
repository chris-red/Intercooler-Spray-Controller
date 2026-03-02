#pragma once

#include <stdbool.h>

/**
 * @brief Initialize the G-meter.
 *
 * Enables the QMI8658 accelerometer and starts tracking.
 */
void G_Meter_Init(void);

/**
 * @brief Call from the driver loop (every ~100 ms) to sample the
 *        accelerometer and update peak tracking.
 */
void G_Meter_Update(void);

/**
 * @brief Get the lateral (left/right) G-force.
 *        Positive = right, negative = left.
 */
float G_Meter_GetLateral(void);

/**
 * @brief Get the longitudinal (forward/backward) G-force.
 *        Positive = forward (accel), negative = backward (braking).
 */
float G_Meter_GetLongitudinal(void);

/**
 * @brief Get the 2D resultant G-force (lateral + longitudinal plane).
 */
float G_Meter_GetCurrent(void);

/* ---- Per-direction peak values ---- */

float G_Meter_GetMaxLeft(void);     /**< Peak left  G (positive value) */
float G_Meter_GetMaxRight(void);    /**< Peak right G (positive value) */
float G_Meter_GetMaxForward(void);  /**< Peak forward/accel G          */
float G_Meter_GetMaxBrake(void);    /**< Peak braking G (positive val) */

/**
 * @brief Reset all four directional maxes to zero.
 */
void G_Meter_ResetMax(void);

/**
 * @brief Restore persisted max values (called from settings_load).
 */
void G_Meter_SetMaxDirectional(float left, float right, float fwd, float brk);

/* ---- Calibration ---- */

/**
 * @brief Calibrate the G-meter by capturing the current resting
 *        accelerometer values as the zero offset.  Must be called
 *        while the device is stationary in its mounted position.
 */
void G_Meter_Calibrate(void);

/**
 * @brief Get the current calibration offsets (Y and Z axes).
 */
void G_Meter_GetCalibration(float *out_y, float *out_z);

/**
 * @brief Restore previously persisted calibration offsets.
 */
void G_Meter_SetCalibration(float cal_y, float cal_z);

/**
 * @brief Return true if a calibration has been applied.
 */
bool G_Meter_IsCalibrated(void);
