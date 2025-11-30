#pragma once

/**
 * Status Bar Widget
 * Displays system information: battery, time, CAN status, etc.
 */

#include "lvgl.h"

class StatusBar {
public:
    /**
     * Create status bar at top of screen
     * @param parent Parent screen
     * @return Status bar object
     */
    static lv_obj_t* create(lv_obj_t* parent);

    /**
     * Update status bar information
     * @param bar Status bar object
     */
    static void update(lv_obj_t* bar);

    /**
     * Set battery level
     * @param bar Status bar object
     * @param percent Battery percentage (0-100)
     * @param charging Is charging
     */
    static void set_battery(lv_obj_t* bar, uint8_t percent, bool charging);

    /**
     * Set CAN status
     * @param bar Status bar object
     * @param connected CAN bus connected
     * @param baudrate Current baud rate
     */
    static void set_can_status(lv_obj_t* bar, bool connected, uint32_t baudrate);

    /**
     * Set time
     * @param bar Status bar object
     * @param hours Hour (0-23)
     * @param minutes Minute (0-59)
     */
    static void set_time(lv_obj_t* bar, uint8_t hours, uint8_t minutes);

private:
    // Child widget indices
    static constexpr int IDX_TIME = 0;
    static constexpr int IDX_CAN_STATUS = 1;
    static constexpr int IDX_BATTERY = 2;
};
