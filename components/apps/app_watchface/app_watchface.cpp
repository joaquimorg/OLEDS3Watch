#include "app_watchface.h"
#include "lvgl.h"
#include <ctime>
#include <cstdio>
#include "assets/esp_brookesia_assets.h"

LV_IMG_DECLARE(img_app_watchface);

WatchFace::WatchFace() : ESP_Brookesia_PhoneApp("WatchFace", &img_app_watchface, true, /*status*/false, /*nav*/false)
{
}

WatchFace::~WatchFace()
{
}

bool WatchFace::run(void)
{
    lv_area_t area = getVisualArea();
    int _width = area.x2 - area.x1;
    int _height = area.y2 - area.y1;

    panel_obj_ = lv_obj_create(lv_scr_act());
    lv_obj_set_size(panel_obj_, _width, _height);
    lv_obj_align(panel_obj_, LV_ALIGN_TOP_LEFT, 0, 0);
    // Black background for AMOLED-friendly watchface
    lv_obj_set_style_bg_color(panel_obj_, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(panel_obj_, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(panel_obj_, lv_color_black(), 0);
    lv_obj_set_style_border_opa(panel_obj_, LV_OPA_COVER, 0);

    lv_obj_add_event_cb(panel_obj_, touch_event_cb, LV_EVENT_PRESSING, this);

    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLL_CHAIN_VER);

    // Time: HH:MM (large) + :SS (smaller) next to it
    time_label_ = lv_label_create(panel_obj_);
    lv_obj_set_style_text_align(time_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(time_label_, &esp_brookesia_font_maison_neue_book_48, 0);
    lv_obj_set_style_text_color(time_label_, lv_color_white(), 0);
    lv_obj_align(time_label_, LV_ALIGN_CENTER, 0, -20);

    sec_label_ = lv_label_create(panel_obj_);
    lv_obj_set_style_text_font(sec_label_, &esp_brookesia_font_maison_neue_book_24, 0);
    lv_obj_set_style_text_color(sec_label_, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align_to(sec_label_, time_label_, LV_ALIGN_OUT_RIGHT_MID, 8, -8);

    // Date label below time
    date_label_ = lv_label_create(panel_obj_);
    lv_obj_set_style_text_align(date_label_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(date_label_, &esp_brookesia_font_maison_neue_book_20, 0);
    lv_obj_set_style_text_color(date_label_, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(date_label_, LV_ALIGN_CENTER, 0, 24);

    // Battery label bottom-right
    batt_label_ = lv_label_create(panel_obj_);
    lv_obj_set_style_text_font(batt_label_, &esp_brookesia_font_maison_neue_book_16, 0);
    lv_obj_set_style_text_color(batt_label_, lv_color_hex(0x8BC34A), 0);
    lv_obj_align(batt_label_, LV_ALIGN_BOTTOM_RIGHT, -12, -14);

    update_datetime_battery();

    // 1 Hz update
    tick_timer_ = lv_timer_create(tick_timer_cb, 1000, this);

    return true;
}

bool WatchFace::back(void)
{
    notifyCoreClosed();

    return true;
}

bool WatchFace::close(void)
{
    if (tick_timer_) {
        lv_timer_del(tick_timer_);
        tick_timer_ = nullptr;
    }
    panel_obj_ = nullptr;
    time_label_ = nullptr;
    sec_label_ = nullptr;
    date_label_ = nullptr;
    batt_label_ = nullptr;
    return true;
}

bool WatchFace::init(void)
{
    return true;
}

void WatchFace::touch_event_cb(lv_event_t *e)
{
    WatchFace *app = (WatchFace *)lv_event_get_user_data(e);
    lv_indev_t *indev = lv_indev_get_act();
    (void)app; (void)indev;
}

void WatchFace::tick_timer_cb(lv_timer_t *timer)
{
    if (!timer || !timer->user_data) return;
    WatchFace *self = static_cast<WatchFace *>(timer->user_data);
    self->update_datetime_battery();
}

static int wf_get_battery_percent()
{
    // TODO: substitute with actual ADC/fuel gauge reading if available
    return 100;
}

void WatchFace::update_datetime_battery()
{
    // Time and date
    time_t now = time(nullptr);
    struct tm tm_now = {};
    localtime_r(&now, &tm_now);

    char time_buf[8];   // HH:MM
    char sec_buf[4];    // :SS or SS
    char date_buf[32];  // e.g., Tue 27 Aug 2024

    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", tm_now.tm_hour, tm_now.tm_min);
    snprintf(sec_buf, sizeof(sec_buf), ":%02d", tm_now.tm_sec);
    strftime(date_buf, sizeof(date_buf), "%a %d %b %Y", &tm_now);

    if (time_label_) lv_label_set_text(time_label_, time_buf);
    if (sec_label_)  lv_label_set_text(sec_label_, sec_buf);
    if (date_label_) lv_label_set_text(date_label_, date_buf);

    // Battery percent
    int batt = wf_get_battery_percent();
    char batt_buf[16];
    snprintf(batt_buf, sizeof(batt_buf), "Batt %d%%", batt);
    if (batt_label_) lv_label_set_text(batt_label_, batt_buf);
}
