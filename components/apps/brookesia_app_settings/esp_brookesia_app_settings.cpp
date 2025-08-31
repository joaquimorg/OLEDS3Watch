#include <iterator>
#include <thread>
#include <unistd.h>
//#include "esp_brookesia_app_settings_utils.hpp"
#include "esp_brookesia_app_settings.hpp"

#define ESP_UTILS_LOG_TAG "BS:App:Settings"
#include "esp_lib_utils.h"

#define APP_NAME "Settings"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

namespace esp_brookesia::apps {

    Settings::Settings() :
        App({
        .name = APP_NAME,
        .launcher_icon = gui::StyleImage::IMAGE(&esp_brookesia_app_icon_launcher_settings_112_112),
        .screen_size = gui::StyleSize::RECT_PERCENT(100, 100),
        .flags = {
            .enable_default_screen = 1,
            .enable_recycle_resource = 0,
            .enable_resize_visual_area = 1,
        },
        },
        {
        .app_launcher_page_index = 0,
        .flags = {
            .enable_navigation_gesture = 1,
        },
        })
    {}

    Settings::~Settings()
    {
        ESP_UTILS_LOGD("Destroy(@0x%p)", this);
    }


    bool Settings::run()
    {
        ESP_UTILS_LOGD("Run");

        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x002020), 0);

        title = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(title, &esp_brookesia_font_maison_neue_book_32, 0);
        //lv_obj_set_style_text_color(title, BOARD_TITLE_COLOR, 0);
        lv_label_set_text(title, "Settings !!!");
        lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

        return true;
    }

    bool Settings::back()
    {
        ESP_UTILS_LOGD("Back");
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool Settings::close()
    {
        ESP_UTILS_LOGD("Close");

        return true;
    }

    bool Settings::init()
    {
        ESP_UTILS_LOGD("Init");

        return true;
    }

    bool Settings::deinit()
    {
        ESP_UTILS_LOGD("Deinit");

        return true;
    }


    ESP_UTILS_REGISTER_PLUGIN(systems::base::App, Settings, APP_NAME)

}