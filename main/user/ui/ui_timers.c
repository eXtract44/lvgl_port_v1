
#include "ui_timers.h"
#include "backlight.h"
#include "ui_blocks.h"
#include "ui_core.h"
#include "ui_theme.h"
#include "ui_time.h"
#include "ui_weather_anim.h"
#include "ui_user_config.h"
#include "wifi_user.h"

extern sgp30_data_t sgp30_data;
volatile int standby_touched = 0; // callback for extern touch driver

void create_timers() {
  lv_timer_create(timer_60000, 60000, &ui); // проверка раз в минуту
  lv_timer_create(timer_10000, 10000, NULL);
  lv_timer_create(timer_1000, 1000, NULL);
  lv_timer_create(timer_200, 200, NULL);
  lv_timer_create(timer_33, 33, &ui);
}

void timer_60000(lv_timer_t *t) {
  ui_main_menu_t *ui = t->user_data;
  if (sgp30_data.init_ok == 0) {
    set_visible(ui->co2.calib_popup, false);
  }
  if ((xTaskGetTickCount() - ui->co2.calib_start_tick) >= CO2_CALIB_TIME) {
#if CONFIG_HAS_SCD41
    lv_obj_add_flag(ui->sensor.tvoc_calib_icon, LV_OBJ_FLAG_HIDDEN);
#else
    lv_obj_add_flag(ui->co2.calib_icon_label, LV_OBJ_FLAG_HIDDEN);
#endif
    lv_timer_del(t);
  }
}

void timer_10000(lv_timer_t *timer) {
  LV_UNUSED(timer);
#if ACTIVATE_BLOCK_TOP_RIGHT
  update_block_top_right(&ui);
#endif

  // --- авто-тема ---
  if (ui.settings.switch_.theme_mode == 0) { // 0 = Auto
    bool is_day = get_is_day();
    if (is_day != ui.settings.switch_.theme_last_is_day) {
      ui.settings.switch_.theme_last_is_day = is_day;
      is_day ? apply_theme_light(&ui) : apply_theme_dark(&ui);
      lv_obj_report_style_change(&ui.style.main);
      lv_obj_report_style_change(&ui.style.popup);
      lv_obj_report_style_change(&ui.style.top_left);
      lv_obj_report_style_change(&ui.style.bot_left);
      lv_obj_report_style_change(&ui.style.top_right);
      lv_obj_report_style_change(&ui.style.bot_right);
      lv_obj_report_style_change(&ui.font.very_small_20);
      lv_obj_report_style_change(&ui.font.small_24);
      lv_obj_report_style_change(&ui.font.medium_32);
      lv_obj_report_style_change(&ui.font.large_48);
      lv_obj_report_style_change(&ui.font.nav_btn);
      lv_obj_report_style_change(&ui.font.time);
      lv_obj_report_style_change(&ui.font.icon);
      lv_obj_report_style_change(&ui.style.category_btn);
    }
  }
}

void timer_1000(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (!ui.settings.switch_.standby_screen_off) {
    switch (ui.settings.switch_.backlight_mode) {
    case 1:
      backlight_set(backlight_get_auto_pct());
      break;
    case 2:
      backlight_set(backlight_get_zeitplan_pct());
      break;
    default:
      break;
    }
  }
#if ACTIVATE_BLOCK_TOP_LEFT
  update_block_top_left(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  update_block_bot_left(&ui);
#endif
#if ACTIVATE_BLOCK_TOP_MID
  update_block_top_middle(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_MID
  update_block_bot_middle(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  update_block_bot_right(&ui);
#endif
}

void timer_200(lv_timer_t *timer) {
  LV_UNUSED(timer);
  display_standby_handle(&ui);
}

void timer_33(lv_timer_t *timer) {
  ui_main_menu_t *ui = timer->user_data;

  if (ui->co2.co2_target < 0)
    return;

  if (ui->co2.co2_display < 0) {
    ui->co2.co2_display = ui->co2.co2_target;
  }

  int32_t diff = ui->co2.co2_target - ui->co2.co2_display;
  if (diff == 0)
    return;

  int32_t step = diff / 8;
  if (step == 0)
    step = (diff > 0) ? 1 : -1;

  ui->co2.co2_display += step;
  uint16_t val = (uint16_t)ui->co2.co2_display;

  // обновляем иглу
  lv_meter_set_indicator_value(ui->co2.meter, ui->co2.indicator, val);

  // обновляем цветной arc от начала до текущего значения
  lv_meter_set_indicator_start_value(ui->co2.meter, ui->co2.needle_arc,
                                     MIN_VALUE_CO2);
  lv_meter_set_indicator_end_value(ui->co2.meter, ui->co2.needle_arc, val);

  // меняем цвет arc в зависимости от значения
  lv_meter_indicator_t *arc = ui->co2.needle_arc;
  arc->type_data.arc.color = calc_co2_color(val);
  lv_obj_invalidate(ui->co2.meter);
}

void display_standby_handle(ui_main_menu_t *ui) {
  static uint32_t timer_standby_sec = 0;

  uint8_t mode = ui->settings.switch_.standby_mode;

  bool standby_active = false;
  if (mode == 1) {
    standby_active = true;
  } else if (mode == 2) {
    standby_active = !get_is_day();
  }

  if (!standby_active) {
    timer_standby_sec = 0;
    ui->settings.switch_.standby_screen_off = false;
    set_visible(ui->standby.background, false);
    return;
  }

  if (is_screen_pressed()) {
    timer_standby_sec = 0;
    ui->settings.switch_.standby_screen_off = false;
    set_visible(ui->standby.background, false);
    backlight_set(backlight_get_current_pct(ui)); // восстанавливаем яркость
  } else {
    timer_standby_sec++;
    if (timer_standby_sec > MAX_STANDBY_TIME * 5) {
      ui->settings.switch_.standby_screen_off = true;
      backlight_set(0); // гасим подсветку
      lv_obj_move_foreground(ui->standby.background);
      set_visible(ui->standby.background, true);
      timer_standby_sec = MAX_STANDBY_TIME * 5;
    }
  }
}

uint8_t backlight_get_current_pct(ui_main_menu_t *ui) {
  switch (ui->settings.switch_.backlight_mode) {
  case 1:
    return backlight_get_auto_pct();
  case 2:
    return backlight_get_zeitplan_pct();
  default:
    return ui->settings.switch_.backlight_pct;
  }
}

bool is_screen_pressed(void) { return standby_touched; }




