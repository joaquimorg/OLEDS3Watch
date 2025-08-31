#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "assets/esp_brookesia_app_settings_assets.h"

namespace esp_brookesia::apps {

    class Settings : public systems::phone::App {
    public:
        Settings();
        ~Settings();

        //SettingsUI ui;
        //SettingsManager manager;

        bool run() override;
        bool back() override;
        bool close() override;
        bool init() override;
        bool deinit() override;

    private:
        lv_obj_t* title;
    };

}
