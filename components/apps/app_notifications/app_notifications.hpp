#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"
#include "assets/app_notifications_assets.h"

namespace esp_brookesia::apps {

    class Notifications : public systems::phone::App {
    public:
        Notifications();
        ~Notifications();

        bool run() override;
        bool back() override;
        bool close() override;
        bool init() override;
        bool deinit() override;

    private:
        lv_obj_t* title;
    };

}
