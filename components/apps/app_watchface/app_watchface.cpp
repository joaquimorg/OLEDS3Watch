#include <iterator>
#include <thread>
#include <unistd.h>
#include "app_watchface.hpp"

#define ESP_UTILS_LOG_TAG "App:Watchface"
#include "esp_lib_utils.h"

#define APP_NAME "Watchface"

using namespace std;
using namespace esp_brookesia::gui;
using namespace esp_brookesia::systems::phone;

namespace esp_brookesia::apps {

    Watchface::Watchface() :
        App({
        .name = APP_NAME,
        .launcher_icon = gui::StyleImage::IMAGE(&app_watchface_128_128),
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

    Watchface::~Watchface()
    {
        ESP_UTILS_LOGD("Destroy(@0x%p)", this);
    }


    bool Watchface::run()
    {
        ESP_UTILS_LOGD("Run");

        lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x200020), 0);

        title = lv_label_create(lv_scr_act());
        lv_obj_set_style_text_font(title, &esp_brookesia_font_maison_neue_book_32, 0);
        //lv_obj_set_style_text_color(title, BOARD_TITLE_COLOR, 0);
        lv_label_set_text(title, "Watchface !!!");
        lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

        return true;
    }

    bool Watchface::back()
    {
        ESP_UTILS_LOGD("Back");
        ESP_UTILS_CHECK_FALSE_RETURN(notifyCoreClosed(), false, "Notify core closed failed");
        return true;
    }

    bool Watchface::close()
    {
        ESP_UTILS_LOGD("Close");

        return true;
    }

    bool Watchface::init()
    {
        ESP_UTILS_LOGD("Init");

        return true;
    }

    bool Watchface::deinit()
    {
        ESP_UTILS_LOGD("Deinit");

        return true;
    }


    ESP_UTILS_REGISTER_PLUGIN(systems::base::App, Watchface, APP_NAME)

}