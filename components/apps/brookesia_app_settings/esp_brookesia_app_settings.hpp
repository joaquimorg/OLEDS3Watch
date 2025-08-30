#pragma once

#include "lvgl.h"
#include "esp_brookesia.hpp"
//#include "esp_brookesia_app_settings_ui.hpp"
#include "assets/esp_brookesia_app_settings_assets.h"

namespace esp_brookesia::apps {

class Settings: public systems::phone::App {
public:
    Settings(const Settings &) = delete;
    Settings(Settings &&) = delete;
    Settings &operator=(const Settings &) = delete;
    Settings &operator=(Settings &&) = delete;

    ~Settings();

    static Settings *requestInstance();

    //SettingsUI ui;
    //SettingsManager manager;

protected:
    bool run() override;
    bool back() override;
    bool close() override;
    bool init() override;
    bool deinit() override;

    bool isStarting() const
    {
        return _is_starting;
    }
    bool isStopping() const
    {
        return _is_stopping;
    }

private:
    Settings();


    std::atomic<bool> _is_starting = false;
    std::atomic<bool> _is_stopping = false;

    inline static Settings *_instance = nullptr;
};

}
