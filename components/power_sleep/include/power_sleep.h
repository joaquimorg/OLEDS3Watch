#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Initialize light-sleep system: configures ext0 wake on GPIO set in menuconfig. */
esp_err_t power_sleep_init(void);

/** Enter light sleep for at least min_sleep_us microseconds (rounded up). */
esp_err_t power_sleep_enter(uint64_t min_sleep_us);

/** Convenience: enter light sleep using Kconfig POWER_SLEEP_MIN_SLEEP_MS. */
esp_err_t power_sleep_enter_default(void);

/** Returns true if the last wakeup was from EXT0 (the wake pin). */
bool power_sleep_woke_by_gpio(void);

/** Optional: configure pull-ups/downs on the wake GPIO (call after init if needed). */
esp_err_t power_sleep_configure_gpio_pulls(bool enable_pullup, bool enable_pulldown);

/** Set display timeout in milliseconds (0 disables auto-sleep). */
void power_sleep_set_display_timeout_ms(uint32_t timeout_ms);

/** Start background task that turns off backlight after timeout and sleeps. */
esp_err_t power_sleep_start_manager(void);

/** Notify user activity (e.g., touch) to reset the inactivity timer. */
void power_sleep_notify_activity(void);

/** Bind global LVGL touch events to notify activity system-wide. */
esp_err_t power_sleep_bind_global_touch_activity(void);

#ifdef __cplusplus
}
#endif
