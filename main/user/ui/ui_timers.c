
#include "ui_timers.h"
#include "backlight.h"
#include "ui_blocks.h"
#include "ui_core.h"
#include "ui_theme.h"
#include "ui_time.h"
#include "ui_user_config.h"
#include "ui_weather_anim.h"
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
  static uint32_t standby_200mS_cnt = 0;
  static int16_t  last_pct = -1;

  bool    is_day  = get_is_day();
  bool    pressed = is_screen_pressed();
  uint8_t mode    = ui.settings.switch_.display_mode;

  bool standby_enabled =
      (mode == DISPLAY_MODE_STANDBY_180S) ||
      (mode == DISPLAY_MODE_NIGHT_STANDBY_180S && !is_day);

  // экран должен быть активен: standby выключен ИЛИ касание
  if (!standby_enabled || pressed) {
    standby_200mS_cnt = 0;
    set_visible(ui.standby.background, false);
    uint8_t pct = backlight_get_current_pct(&ui);
    if ((int16_t)pct != last_pct) { backlight_set(pct); last_pct = pct; }
    return;
  }

  // standby активен, касания нет
  if (standby_200mS_cnt <= MAX_STANDBY_TIME) {
    standby_200mS_cnt++;                                  // фаза отсчёта — экран ещё жив
    uint8_t pct = backlight_get_current_pct(&ui);
    if ((int16_t)pct != last_pct) { backlight_set(pct); last_pct = pct; }
  } else {                                          // сон
    if (last_pct != 0) { backlight_set(0); last_pct = 0; }
    lv_obj_move_foreground(ui.standby.background);
    set_visible(ui.standby.background, true);
  }
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

uint8_t backlight_get_current_pct(ui_main_menu_t *ui) {
	if(ui->settings.switch_.backlight_mode == BACKLIGHT_MODE_MANUELL){
		return ui->settings.switch_.backlight_manual_pct;
	}else if(ui->settings.switch_.backlight_mode == BACKLIGHT_MODE_AUTO){
		return backlight_get_auto_pct();
	}else if(ui->settings.switch_.backlight_mode == BACKLIGHT_MODE_ZEITPLAN){
		return backlight_get_zeitplan_pct(); 
	}else{
		return ui->settings.switch_.backlight_manual_pct;
	}
}

bool is_screen_pressed(void) { return standby_touched; }
