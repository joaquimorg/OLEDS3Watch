#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"

class WatchFace: public ESP_Brookesia_PhoneApp
{
public:
    WatchFace();
    ~WatchFace();

    bool run(void);
    bool back(void);
    bool close(void);

    bool init(void) override;

private:
    static void touch_event_cb(lv_event_t *e);

    static void tick_timer_cb(lv_timer_t *timer);
    void update_datetime_battery();

    lv_obj_t *panel_obj_ = nullptr;
    lv_obj_t *time_label_ = nullptr;
    lv_obj_t *sec_label_ = nullptr;
    lv_obj_t *date_label_ = nullptr;
    lv_obj_t *batt_label_ = nullptr;
    lv_timer_t *tick_timer_ = nullptr;
};
