#include "power_sleep.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "bsp/display.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef CONFIG_POWER_SLEEP_GPIO_WAKE
#define CONFIG_POWER_SLEEP_GPIO_WAKE 0
#endif
#ifndef CONFIG_POWER_SLEEP_EXT0_LEVEL
#define CONFIG_POWER_SLEEP_EXT0_LEVEL 0
#endif
#ifndef CONFIG_POWER_SLEEP_MIN_SLEEP_MS
#define CONFIG_POWER_SLEEP_MIN_SLEEP_MS 100
#endif

static const char *TAG = "power_sleep";

static TaskHandle_t s_mgr_task = NULL;
static uint32_t s_display_timeout_ms = CONFIG_POWER_SLEEP_DISPLAY_TIMEOUT_MS;
static volatile bool s_activity_notified = false;
static bool s_touch_hook_bound = false;

static void power_sleep_lvgl_touch_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_PRESSED || code == LV_EVENT_PRESSING || code == LV_EVENT_CLICKED) {
        power_sleep_notify_activity();
    }
}

static void power_sleep_lvgl_screen_loaded_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    if (!scr) return;
    // Bind touch cb to the new screen as well
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_CLICKED, NULL);
    // Keep listening for future loads
    lv_obj_add_event_cb(scr, power_sleep_lvgl_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
}

esp_err_t power_sleep_init(void)
{
    gpio_num_t wake_gpio = (gpio_num_t)CONFIG_POWER_SLEEP_GPIO_WAKE;

    // Configure the wake GPIO as input (keep pull state configurable)
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << wake_gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&io), TAG, "gpio_config failed");

    // Clear any previous wake sources and enable EXT0 on the selected pin/level
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    ESP_RETURN_ON_ERROR(
        esp_sleep_enable_ext0_wakeup(
            wake_gpio,
            (CONFIG_POWER_SLEEP_EXT0_LEVEL ? 1 : 0)
        ), TAG, "enable ext0 failed");

    ESP_LOGI(TAG, "Light sleep EXT0 wake on GPIO%d level %d", wake_gpio, CONFIG_POWER_SLEEP_EXT0_LEVEL);
    return ESP_OK;
}

esp_err_t power_sleep_configure_gpio_pulls(bool enable_pullup, bool enable_pulldown)
{
    gpio_num_t wake_gpio = (gpio_num_t)CONFIG_POWER_SLEEP_GPIO_WAKE;
    ESP_RETURN_ON_ERROR(gpio_set_pull_mode(wake_gpio,
        enable_pullup ? GPIO_PULLUP_ONLY : (enable_pulldown ? GPIO_PULLDOWN_ONLY : GPIO_FLOATING)), TAG, "pull set failed");
    return ESP_OK;
}

bool power_sleep_woke_by_gpio(void)
{
    esp_sleep_wakeup_cause_t c = esp_sleep_get_wakeup_cause();
    return (c == ESP_SLEEP_WAKEUP_EXT0);
}

esp_err_t power_sleep_enter(uint64_t min_sleep_us)
{
    if (min_sleep_us == 0) {
        min_sleep_us = 1000ULL * CONFIG_POWER_SLEEP_MIN_SLEEP_MS;
    }

    // Prepare for light sleep: flush logs and allow peripherals to idle.
    ESP_LOGI(TAG, "Entering light sleep >= %llu us (wake on GPIO%d level %d)",
             (unsigned long long)min_sleep_us, CONFIG_POWER_SLEEP_GPIO_WAKE, CONFIG_POWER_SLEEP_EXT0_LEVEL);

    // Note: When using USB-CDC/JTAG, light sleep will pause UART output; that's normal.
    // If needed, stop high-frequency peripherals here (I2S, display refresh, etc.).

    // Request light sleep for at least the specified duration.
    esp_err_t err = esp_light_sleep_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_light_sleep_start failed: %s", esp_err_to_name(err));
        return err;
    }

    // Woken up
    if (power_sleep_woke_by_gpio()) {
        ESP_LOGI(TAG, "Woke from EXT0 (GPIO%d)", CONFIG_POWER_SLEEP_GPIO_WAKE);
    } else {
        ESP_LOGI(TAG, "Woke from cause %d", esp_sleep_get_wakeup_cause());
    }
    return ESP_OK;
}

esp_err_t power_sleep_enter_default(void)
{
    return power_sleep_enter(1000ULL * CONFIG_POWER_SLEEP_MIN_SLEEP_MS);
}

void power_sleep_set_display_timeout_ms(uint32_t timeout_ms)
{
    s_display_timeout_ms = timeout_ms;
}

static void power_sleep_manager_task(void *arg)
{
    // Ensure backlight starts ON
    bsp_display_backlight_on();

    while (1) {
        uint32_t to = s_display_timeout_ms;
        if (to == 0) {
            // Auto-sleep disabled: just idle a while
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        // Wait for inactivity with reset on activity notification
        TickType_t remain = pdMS_TO_TICKS(to);
        for (;;) {
            uint32_t got = ulTaskNotifyTake(pdTRUE, remain);
            if (got > 0 || s_activity_notified) {
                // Activity: restart countdown
                s_activity_notified = false;
                remain = pdMS_TO_TICKS(to);
                continue;
            }
            // Timed out without activity
            break;
        }

        // Turn off backlight and sleep until GPIO wake
        ESP_LOGI(TAG, "Display timeout reached (%ums). Backlight OFF, entering light sleep...", (unsigned)to);
        bsp_display_backlight_off();

        // Enter light sleep and wait for EXT0 (GPIO)
        esp_err_t err = esp_light_sleep_start();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_light_sleep_start failed: %s", esp_err_to_name(err));
        }

        // Wake up: turn backlight on
        if (power_sleep_woke_by_gpio()) {
            ESP_LOGI(TAG, "Wake by GPIO%d. Backlight ON.", CONFIG_POWER_SLEEP_GPIO_WAKE);
        } else {
            ESP_LOGI(TAG, "Wake cause %d. Backlight ON.", esp_sleep_get_wakeup_cause());
        }
        bsp_display_backlight_on();

        // Loop restarts; timer runs again
    }
}

esp_err_t power_sleep_start_manager(void)
{
    if (s_mgr_task) {
        return ESP_OK;
    }
    // Make sure EXT0 wake is configured
    ESP_RETURN_ON_ERROR(power_sleep_init(), TAG, "init failed");
    // Create background task
    BaseType_t ok = xTaskCreate(
        power_sleep_manager_task,
        "pwr_slp_mgr",
        4096,
        NULL,
        5,
        &s_mgr_task
    );
    if (ok != pdPASS) {
        ESP_LOGE(TAG, "Failed to create manager task");
        return ESP_FAIL;
    }
    return ESP_OK;
}

void power_sleep_notify_activity(void)
{
    s_activity_notified = true;
    if (s_mgr_task) {
        xTaskNotifyGive(s_mgr_task);
    }
}

esp_err_t power_sleep_bind_global_touch_activity(void)
{
    if (s_touch_hook_bound) {
        return ESP_OK;
    }
    lv_obj_t *scr = lv_scr_act();
    if (!scr) {
        ESP_LOGW(TAG, "LVGL not ready to bind touch hook");
        return ESP_ERR_INVALID_STATE;
    }
    // Bind to current screen
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(scr, power_sleep_lvgl_touch_cb, LV_EVENT_CLICKED, NULL);
    // Re-bind automatically when a new screen is loaded
    lv_obj_add_event_cb(scr, power_sleep_lvgl_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    s_touch_hook_bound = true;
    return ESP_OK;
}
