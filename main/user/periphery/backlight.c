// backlight.c
#include "backlight.h"
#include "driver/ledc.h"
#include <math.h>
#include "../periphery/time_user.h"
#include "../periphery/open_meteo.h"
#include "sensors.h"


#define BACKLIGHT_GPIO      GPIO_NUM_6
#define BACKLIGHT_FREQ_HZ   1000
#define BACKLIGHT_RESOLUTION LEDC_TIMER_10_BIT   // 1024 шага
#define BACKLIGHT_TIMER     LEDC_TIMER_0
#define BACKLIGHT_CHANNEL   LEDC_CHANNEL_0


#define BL_DUTY_FORBID_LO   510   // ~35% от 1023
#define BL_DUTY_FORBID_HI   560   // ~55% от 1023


static volatile uint8_t bl_cur = 99;
static volatile uint8_t bl_tgt = 100;



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

void backlight_deinit(void) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
    ledc_stop(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, 0);
}


void backlight_set(uint8_t pct) {
    if (pct > 100) pct = 100;
    bl_tgt = pct;
}

void backlight_tick(void) {     // вызывать из таймера 33 мс
    if (bl_cur == bl_tgt) return;
    bl_cur += (bl_cur < bl_tgt) ? 1 : -1;
    backlight_apply(bl_cur);
}

void backlight_apply(uint8_t percent) {
	if (percent == 0) {                 // полное выключение
        ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
        return;
    }
       if (percent > 100) percent = 100;
    // маппинг 1–100% → 30–100%
    uint8_t mapped = 30 + (percent * 70) / 100;

    float normalized = mapped / 100.0f;
    float corrected  = powf(normalized, 2.2f);
    uint32_t duty    = (uint32_t)(corrected * 1023.0f);
    
      // снап мимо опасной полосы скважности
    if (duty > BL_DUTY_FORBID_LO && duty < BL_DUTY_FORBID_HI) {
        duty = (duty < (BL_DUTY_FORBID_LO + BL_DUTY_FORBID_HI) / 2)
                   ? BL_DUTY_FORBID_LO
                   : BL_DUTY_FORBID_HI;
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BACKLIGHT_CHANNEL);
}

uint8_t backlight_get_auto_pct(uint8_t min_pct, uint8_t max_pct) {
    return get_is_day() ? max_pct : min_pct;
}

// ──────────────────────────────────────────────
// Approximate sunrise/sunset for ~51°N (Germany)
// Values in minutes from midnight
// ──────────────────────────────────────────────
typedef struct {
    uint16_t sunrise_min;
    uint16_t sunset_min;
} sun_schedule_t;

// Index 0 = January, 11 = December
static const sun_schedule_t sun_schedule[12] = {
    {8*60+15, 16*60+30}, // Jan
    {7*60+45, 17*60+30}, // Feb
    {6*60+45, 18*60+20}, // Mar
    {5*60+30, 19*60+15}, // Apr
    {4*60+30, 20*60+10}, // May
    {4*60+00, 20*60+50}, // Jun
    {4*60+20, 20*60+40}, // Jul
    {5*60+15, 19*60+45}, // Aug
    {6*60+10, 18*60+30}, // Sep
    {7*60+10, 17*60+10}, // Oct
    {7*60+15, 16*60+20}, // Nov  (after DST end)
    {8*60+05, 16*60+10}, // Dec
};

#define BACKLIGHT_ZEITPLAN_PCT_MIN        5
#define BACKLIGHT_ZEITPLAN_PCT_MAX        100
#define BACKLIGHT_ZEITPLAN_TRANSITION_MIN 90

uint8_t backlight_get_zeitplan_pct(void) {
    uint8_t  h     = get_time_hour();
    uint8_t  m     = get_time_minute();
    uint8_t  month = get_time_month();   // 1..12
    uint8_t  day   = get_time_mday();     // 1..31 (reserved for future use)

    (void)day; // not used in variant A, suppress warning

    uint16_t now     = (uint16_t)h * 60 + m;
    uint16_t sunrise = sun_schedule[month - 1].sunrise_min;
    uint16_t sunset  = sun_schedule[month - 1].sunset_min;

    uint16_t t = BACKLIGHT_ZEITPLAN_TRANSITION_MIN;

    // Night → Dawn transition
    if (now >= sunrise && now < sunrise + t) {
        uint8_t progress = (uint8_t)((now - sunrise) * 100 / t);
        return (uint8_t)(BACKLIGHT_ZEITPLAN_PCT_MIN +
               (progress * (BACKLIGHT_ZEITPLAN_PCT_MAX - BACKLIGHT_ZEITPLAN_PCT_MIN)) / 100);
    }

    // Dusk → Night transition
    if (now >= sunset && now < sunset + t) {
        uint8_t progress = (uint8_t)((now - sunset) * 100 / t);
        return (uint8_t)(BACKLIGHT_ZEITPLAN_PCT_MAX -
               (progress * (BACKLIGHT_ZEITPLAN_PCT_MAX - BACKLIGHT_ZEITPLAN_PCT_MIN)) / 100);
    }

    // Full day
    if (now >= sunrise + t && now < sunset) {
        return BACKLIGHT_ZEITPLAN_PCT_MAX;
    }

    // Night
    return BACKLIGHT_ZEITPLAN_PCT_MIN;
}