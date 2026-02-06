# Intercooler Controller UI - Getting Started Guide

## Overview

I've created a modular UI system for your intercooler sprayer controller. This document explains the structure and how to use it.

## Files Created

### 1. `intercooler_ui.h` - Header File
This contains:
- **Color Constants** - All UI colors defined as `#define` macros for easy customization
- **Function Declarations** - Public functions you call to manage the UI

### 2. `intercooler_ui.c` - Implementation File
This contains the implementation of all UI functions and manages:
- Screen layout
- Widget creation
- Timer-based updates

## Main Screen Components

The UI has already been integrated into your demo and displays:

1. **Temperature Display (Large)** - Shows current temperature in °C
   - Changes color based on temperature:
     - Green: Normal (< 40°C)
     - Orange: Warning (40-50°C)
     - Red: Critical (≥ 50°C)

2. **Time Display** - Shows current time from RTC
   - Updates every second automatically

3. **Relay Indicator** - Visual LED indicator
   - Off (dark gray) when relay is inactive
   - On (green) when relay is triggered

4. **Tank Indicator** - Visual LED indicator
   - Off (dark gray) when tank is OK
   - On (red) when tank is empty

## Color Customization

All colors are defined as constants in `intercooler_ui.h`. You can easily change them:

```c
#define COLOR_BG_PRIMARY        lv_color_hex(0x1a1a2e)  // Background
#define COLOR_TEMP_NORMAL       lv_color_hex(0x00FF00)  // Green
#define COLOR_TEMP_WARNING      lv_color_hex(0xFFA500)  // Orange
#define COLOR_TEMP_CRITICAL     lv_color_hex(0xFF0000)  // Red
#define COLOR_RELAY_ACTIVE      lv_color_hex(0x00FF00)  // Green
#define COLOR_TANK_EMPTY        lv_color_hex(0xFF0000)  // Red
```

Just edit the hex values (0xRRGGBB format) and rebuild.

## API Functions

### 1. Initialize the UI
```c
intercooler_ui_create();
```
Call this once in `app_main()` to set up the UI. ✓ Already done in main.c

### 2. Update Temperature
```c
intercooler_ui_update_temperature(45.2);  // Display 45.2°C
```
Call this whenever you read a new temperature value from your sensor.
Color changes automatically based on the value.

### 3. Update Time
```c
intercooler_ui_update_time();
```
The UI timer calls this automatically every second, but you can call it manually if needed.

### 4. Set Tank Empty Indicator
```c
intercooler_ui_set_tank_empty(true);   // Show red empty indicator
intercooler_ui_set_tank_empty(false);  // Show tank OK
```

### 5. Set Relay Active Indicator
```c
intercooler_ui_set_relay_active(true);   // Show relay is spraying
intercooler_ui_set_relay_active(false);  // Show relay is off
```

### 6. Cleanup (Optional)
```c
intercooler_ui_cleanup();
```
Call when exiting the UI to clean up resources.

## Testing the UI

Since you don't have hardware yet, here's how to test the UI:

1. **Build and Flash** - Current code already builds and displays the UI
2. **Test Functions** - You can manually call update functions from a terminal or modify `main.c`:

Example test code to add to `Driver_Loop()`:
```c
static int test_counter = 0;
if (test_counter++ % 50 == 0) {
    float test_temp = 25.0 + (test_counter / 50) % 40;  // Cycle through temps
    intercooler_ui_update_temperature(test_temp);
}
```

## Understanding the Code Structure

### Why Modular Design?
- **Separation of Concerns**: UI code is separate from driver code
- **Easy to Modify**: Change colors and layout without touching other code
- **Scalable**: Easy to add new screens or features later

### Font Sizes
Currently using available fonts:
- `lv_font_montserrat_16` - Large (temperature)
- `lv_font_montserrat_14` - Normal (time)
- `lv_font_montserrat_12` - Small (labels)

You can enable larger fonts in the LVGL configuration if needed.

### LVGL Concepts (Quick Reference for Java Developers)

If you're familiar with Java, think of LVGL like this:
- `lv_obj_t` ≈ Java's `Component` or `View`
- `lv_color_hex()` ≈ Java's `Color.parseColor()`
- `lv_obj_set_style_*()` ≈ Java's `setStyle()` or `setAttribute()`
- `lv_timer_create()` ≈ Java's `Timer` or `ScheduledExecutorService`

## Next Steps

Once you have hardware, integrate the UI updates:

1. **Create a sensor reading function** that calls:
   ```c
   intercooler_ui_update_temperature(sensor_value);
   ```

2. **Create a relay control function** that calls:
   ```c
   intercooler_ui_set_relay_active(relay_is_on);
   ```

3. **Create a tank sensor function** that calls:
   ```c
   intercooler_ui_set_tank_empty(tank_empty_pin_value);
   ```

4. **Call these functions** from your `Driver_Loop()` task at appropriate intervals

## File Organization

```
main/
├── main.c                    (Updated to call intercooler_ui_create)
├── CMakeLists.txt           (Updated to include intercooler_ui.c)
└── LVGL_UI/
    ├── LVGL_Example.c       (Original example - can be deleted later)
    ├── LVGL_Example.h
    ├── intercooler_ui.c     (NEW - Your UI implementation)
    └── intercooler_ui.h     (NEW - Your UI interface)
```

## Troubleshooting

**Q: How do I make the temperature text bigger?**
A: Change `font_large` in `intercooler_ui_create()` to a larger font (once you enable it in LVGL config)

**Q: How do I change the background color?**
A: Edit `COLOR_BG_PRIMARY` in `intercooler_ui.h` and rebuild

**Q: The indicators aren't working?**
A: Make sure you're calling `intercooler_ui_set_relay_active()` and `intercooler_ui_set_tank_empty()` with the correct boolean values

**Q: Can I add more UI elements?**
A: Yes! The pattern is straightforward - create new static `lv_obj_t*` variables, initialize them in `intercooler_ui_create()`, and create update functions as needed

Happy coding! Let me know if you'd like to add more features to the UI.
