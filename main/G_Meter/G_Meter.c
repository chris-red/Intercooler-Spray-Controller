#include "G_Meter.h"
#include "QMI8658.h"

#include <math.h>
#include "esp_log.h"

static const char *TAG = "G_Meter";

/*
 * Axis mapping for MOUNTED orientation (board vertical):
 *   Accel.x → gravity axis (vertical when mounted, ignored)
 *   Accel.y → lateral   (positive = right)
 *   Accel.z → longitudinal (positive = forward / acceleration)
 */
static float s_lateral  = 0.0f;
static float s_longit   = 0.0f;
static float s_current_g = 0.0f;

/* Per-direction peak values (all stored as positive magnitudes) */
static float s_max_left    = 0.0f;
static float s_max_right   = 0.0f;
static float s_max_forward = 0.0f;
static float s_max_brake   = 0.0f;

/* Calibration offsets (subtracted from raw readings) */
static float s_cal_y = 0.0f;
static float s_cal_z = 0.0f;
static bool  s_calibrated = false;

void G_Meter_Init(void)
{
    QMI8658_Init();
    ESP_LOGI(TAG, "G-Meter initialised (QMI8658 accel enabled)");
}

void G_Meter_Update(void)
{
    /* Read latest accelerometer values (already in g units via accelScales) */
    getAccelerometer();

    /*
     * Mounted orientation: X = gravity (ignore), Y = lateral, Z = longitudinal.
     * Subtract calibration offsets so the resting state reads 0,0.
     */
    s_lateral = Accel.y - s_cal_y;
    s_longit  = Accel.z - s_cal_z;

    /* 2D resultant in the lateral/longitudinal plane */
    s_current_g = sqrtf(s_lateral * s_lateral + s_longit * s_longit);

    /* Track per-direction peaks */
    if (s_lateral < 0.0f) {                       /* left */
        float mag = -s_lateral;
        if (mag > s_max_left)  s_max_left = mag;
    } else {                                       /* right */
        if (s_lateral > s_max_right) s_max_right = s_lateral;
    }

    if (s_longit > 0.0f) {                        /* forward / accel */
        if (s_longit > s_max_forward) s_max_forward = s_longit;
    } else {                                       /* braking */
        float mag = -s_longit;
        if (mag > s_max_brake) s_max_brake = mag;
    }
}

float G_Meter_GetLateral(void)      { return s_lateral; }
float G_Meter_GetLongitudinal(void) { return s_longit;  }
float G_Meter_GetCurrent(void)      { return s_current_g; }

float G_Meter_GetMaxLeft(void)      { return s_max_left;    }
float G_Meter_GetMaxRight(void)     { return s_max_right;   }
float G_Meter_GetMaxForward(void)   { return s_max_forward; }
float G_Meter_GetMaxBrake(void)     { return s_max_brake;   }

void G_Meter_ResetMax(void)
{
    s_max_left    = 0.0f;
    s_max_right   = 0.0f;
    s_max_forward = 0.0f;
    s_max_brake   = 0.0f;
    ESP_LOGI(TAG, "All directional max G values reset");
}

void G_Meter_SetMaxDirectional(float left, float right, float fwd, float brk)
{
    if (left  > s_max_left)    s_max_left    = left;
    if (right > s_max_right)   s_max_right   = right;
    if (fwd   > s_max_forward) s_max_forward = fwd;
    if (brk   > s_max_brake)   s_max_brake   = brk;
}

/* ---- Calibration ---- */

void G_Meter_Calibrate(void)
{
    /* Take a fresh reading and store Y/Z as the resting offsets */
    getAccelerometer();
    s_cal_y = Accel.y;
    s_cal_z = Accel.z;
    s_calibrated = true;
    ESP_LOGI(TAG, "Calibrated: cal_y=%.4f  cal_z=%.4f", s_cal_y, s_cal_z);
}

void G_Meter_GetCalibration(float *out_y, float *out_z)
{
    if (out_y) *out_y = s_cal_y;
    if (out_z) *out_z = s_cal_z;
}

void G_Meter_SetCalibration(float cal_y, float cal_z)
{
    s_cal_y = cal_y;
    s_cal_z = cal_z;
    s_calibrated = true;
    ESP_LOGI(TAG, "Calibration restored: cal_y=%.4f  cal_z=%.4f", s_cal_y, s_cal_z);
}

bool G_Meter_IsCalibrated(void)
{
    return s_calibrated;
}
