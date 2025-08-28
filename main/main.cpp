/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_check.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "bsp/touch.h"
#include "esp_brookesia.hpp"
#include "bsp_board_extra.h"
#include "display_manager.h"
// Power management
#include "esp_pm.h"

#include "apps.h"

#include "dark/stylesheet.h"

static const char *TAG = "main";

static void my_rounder_cb(lv_disp_drv_t *disp_drv, lv_area_t *area)
{
    area->x1 = (area->x1 >> 1) << 1;
    area->y1 = (area->y1 >> 1) << 1;
    area->x2 = ((area->x2 >> 1) << 1) + 1;
    area->y2 = ((area->y2 >> 1) << 1) + 1;
}

extern "C" void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(bsp_spiffs_mount());
    ESP_LOGI(TAG, "SPIFFS mount successfully");

    //ESP_ERROR_CHECK(bsp_extra_codec_init());

    // Block light-sleep during boot and UI/display bring-up
    display_manager_pm_early_init();

    bsp_extra_init();

    lv_display_t *disp = bsp_display_start();

    if (disp && disp->driver) {
        disp->driver->rounder_cb = my_rounder_cb;
    }
    // if (disp != NULL)
    // {
    //     bsp_display_rotate(disp, LV_DISPLAY_ROTATION_180);
    // }

    ESP_ERROR_CHECK(power_sleep_init());

    bsp_display_lock(0);

    ESP_Brookesia_Phone *phone = new ESP_Brookesia_Phone(disp);
    assert(phone != nullptr && "Failed to create SmartWatch");

    ESP_Brookesia_PhoneStylesheet_t *stylesheet = nullptr;
    stylesheet = new ESP_Brookesia_PhoneStylesheet_t ESP_BROOKESIA_PHONE_DEFAULT_DARK_STYLESHEET();
    ESP_BROOKESIA_CHECK_NULL_EXIT(stylesheet, "Create stylesheet failed");

    if (stylesheet != nullptr)
    {
        ESP_LOGI(TAG, "Using stylesheet (%s)", stylesheet->core.name);
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->addStylesheet(stylesheet), "Add stylesheet failed");
        ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->activateStylesheet(stylesheet), "Activate stylesheet failed");
        delete stylesheet;
    }

    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->setTouchDevice(bsp_display_get_input_dev()), "Set touch device failed");
    phone->registerLvLockCallback((ESP_Brookesia_LvLockCallback_t)(bsp_display_lock), 0);
    phone->registerLvUnlockCallback((ESP_Brookesia_LvUnlockCallback_t)(bsp_display_unlock));
    ESP_BROOKESIA_CHECK_FALSE_EXIT(phone->begin(), "Begin failed");

    // Bind global LVGL touch activity to reset display timeout system-wide
    //ESP_ERROR_CHECK(power_sleep_bind_global_touch_activity());

    // Start display auto-sleep manager (timeout configurable in menuconfig)
    //ESP_ERROR_CHECK(power_sleep_start_manager());

    //Calculator *calculator = new Calculator();
    //assert(calculator != nullptr && "Failed to create calculator");
    //assert((phone->installApp(calculator) >= 0) && "Failed to begin calculator");

    WatchFace *watchface = new WatchFace();
    assert(watchface != nullptr && "Failed to create watchface");
    assert((phone->installApp(watchface) >= 0) && "Failed to begin watchface");


    bsp_display_unlock();

    display_manager_init();

    // Now enable PM with light sleep allowed (still blocked while screen is ON)
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz = 240,
        .min_freq_mhz = 80,
        .light_sleep_enable = true,
    };
    (void)esp_pm_configure(&pm_cfg);
}
