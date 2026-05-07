// backlight.c
#include "backlight.h"
#include "driver/ledc.h"
#include <math.h>
#include "../periphery/periphery.h"

#include "../periphery/open_meteo.h"


#define BACKLIGHT_GPIO      GPIO_NUM_6
#define BACKLIGHT_FREQ_HZ   60000
#define BACKLIGHT_RESOLUTION LEDC_TIMER_10_BIT   // 1024 шага
#define BACKLIGHT_TIMER     LEDC_TIMER_0
#define BACKLIGHT_CHANNEL   LEDC_CHANNEL_0

void backlight_init(void) {
    ledc_timer_config_t timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = BACKLIGHT_TIMER,
        .duty_resolution  = BACKLIGHT_RESOLUTION,
        .freq_hz          = BACKLIGHT_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = BACKLIGHT_CHANNEL,
        .timer_sel      = BACKLIGHT_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BACKLIGHT_GPIO,
        .duty           = 0,
        .hpoint         = 0,
    };
    ledc_channel_config(&channel);
}

void backlight_set(uint8_t percent) {
       if (percent > 100) percent = 100;
    if (percent < 1)   percent = 1;

    // маппинг 1–100% → 20–100%
    uint8_t mapped = 20 + (percent * 80) / 100;

    float normalized = mapped / 100.0f;
    float corrected  = powf(normalized, 2.2f);
    uint32_t duty    = (uint32_t)(corrected * 1023.0f);

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
}

uint8_t backlight_get_auto_pct(void) {
    return get_is_day() ? 80 : 5;
}

uint8_t backlight_get_zeitplan_pct(void) {
    uint8_t h = get_time_hour();

    if (h < 6)  return 5;
    if (h < 8)  return 10 + ((h * 60 - 6 * 60) * (80 - 10)) / (2 * 60);
    if (h < 20) return 80;
    if (h < 22) return 80 - ((h * 60 - 20 * 60) * (80 - 10)) / (2 * 60);
    return 5;
}