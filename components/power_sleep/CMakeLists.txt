#
# Power/Sleep component build file
#

idf_component_register(
    SRCS "power_sleep.c"
    INCLUDE_DIRS "include"
    REQUIRES driver esp_timer esp32_s3_touch_amoled_2_06 lvgl__lvgl
)
