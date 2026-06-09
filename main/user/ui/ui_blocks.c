
#include "ui_blocks.h"
#include "ui_core.h"
#include "ui_popup_settings.h"
#include "ui_time.h"
#include "ui_user_config.h"
#include "ui_weather_anim.h"
#include "ui_widgets.h"
#include "user/periphery/time_user.h"

extern forecast_data_t forecast_data;
extern sgp30_data_t sgp30_data;
#if CONFIG_HAS_SCD41
extern scd41_data_t scd41_data;
#endif
extern sht41_data_t sht41_data;

LV_IMG_DECLARE(sun_48_48);
LV_IMG_DECLARE(moon_42_42);
LV_IMG_DECLARE(cloud_small_70_35);
LV_IMG_DECLARE(cloud_mid_90_45);
LV_IMG_DECLARE(cloud_big_110_50);
LV_IMG_DECLARE(cloud_thin_80_30);
LV_IMG_DECLARE(wind_60_50);
LV_IMG_DECLARE(rain_drop_heavy_9_22);
LV_IMG_DECLARE(snow_flake_2_15_15);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block All
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_menu(ui_main_menu_t *ui) {

  ui->screen = lv_obj_create(NULL);
  lv_obj_add_style(ui->screen, &ui->style.main, 0);
  lv_obj_set_size(ui->screen, LVGL_PORT_H_RES, LVGL_PORT_V_RES);

  ui->standby.background = lv_obj_create(ui->screen);
  lv_obj_add_style(ui->standby.background, &ui->style.main, 0);
  lv_obj_set_size(ui->standby.background, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_move_foreground(ui->standby.background); // ← всегда поверх всего
  lv_obj_clear_flag(ui->standby.background, LV_OBJ_FLAG_SCROLLABLE);
  set_visible(ui->standby.background, false);

#if ACTIVATE_BLOCK_TOP_LEFT
  create_block_top_left(ui);
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  create_block_bot_left(ui);
#endif
#if ACTIVATE_BLOCK_TOP_MID
  create_block_top_middle(ui);
#endif
#if ACTIVATE_BLOCK_TOP_RIGHT
  create_block_top_right(ui);
#endif
#if ACTIVATE_BLOCK_BOT_MID
  create_block_bot_middle(ui);
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  create_block_bot_right(ui);
#endif
}

void update_symbol_wifi(ui_main_menu_t *ui) {
  //  static uint8_t status_wifi_old = 254;
  //  uint8_t current_status = get_wifi_status();
  //  if (current_status == status_wifi_old)
  //    return;
  //
  //  if (!ui->time.wifi_status_icon)
  //    return;
  //
  //  switch (current_status) {
  //  case WIFI_RECONNECT:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_REFRESH);
  //    break;
  //  case WIFI_CONNECTED:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_WIFI);
  //    break;
  //  case WIFI_DISCONNECTED:
  //  default:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_WARNING);
  //    break;
  //  }
  //  status_wifi_old = current_status;
}

float calc_dew_point(float temp, float humidity) {
  if (humidity < 1.0f)
    humidity = 1.0f; // защита от log(0)
  if (humidity > 100.0f)
    humidity = 100.0f;
  const float a = 17.27f;
  const float b = 237.7f;
  float alpha = ((a * temp) / (b + temp)) + logf(humidity / 100.0f);
  return (b * alpha) / (a - alpha);
}

float calc_heat_index(float t, float rh) {
  if (t < 27.0f)
    return t; // heat index не актуален при прохладе
  float hi = -8.78469f + 1.61139f * t + 2.33855f * rh - 0.14611f * t * rh -
             0.01230f * t * t - 0.01642f * rh * rh + 0.00221f * t * t * rh +
             0.00072f * t * rh * rh - 0.000003582f * t * t * rh * rh;
  return hi;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Top Left — Sensor Temperature Humidity
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_top_left(ui_main_menu_t *ui) {
  lv_obj_t *bg =
      create_background(ui->screen, BLOCK_TOP_LEFT_WIDTH, BLOCK_TOP_LEFT_HEIGHT,
                        BLOCK_TOP_LEFT_ALIGN_BACKGROUND, BLOCK_TOP_LEFT_X_START,
                        BLOCK_TOP_LEFT_Y_START);
  if (!bg)
    return;
  ui->sensor.screen = bg;
  lv_obj_add_style(ui->sensor.screen, &ui->style.top_left, 0);
  lv_obj_set_scrollbar_mode(ui->sensor.screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(ui->sensor.screen,
                      block_top_left_open_popup_event_handler, LV_EVENT_CLICKED,
                      ui);

  // --- заголовок ---
  create_text("innen", ui->sensor.screen, STYLE_TEXT_SMALL,
              BLOCK_TOP_LEFT_ALIGN_TITLE, 0, BLOCK_TOP_LEFT_Y_START_TITLE, ui);

  // ═══════════════════════════════════════════
  // СТРОКА 1 — температура (лево) + влажность (право)
  // ═══════════════════════════════════════════

  // иконка температуры
  lv_obj_t *icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_1, MY_TEMPERATURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_temperature = icon;

  // значение температуры
  lv_obj_t *value = create_label(
      ui->sensor.screen, "0°C", BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES, BLOCK_TOP_LEFT_Y_START_VALUE_1 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.temperature_label = value;

  // иконка влажности (правая колонка строки 1)
  icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS_2_RIGHT,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_1, MY_HUMIDITY_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_humidity = icon;

  // значение влажности
  value = create_label(ui->sensor.screen, "0%", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES_R,
                       BLOCK_TOP_LEFT_Y_START_VALUE_1 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.humidity_label = value;

value = create_label(ui->sensor.screen, "", BLOCK_TOP_LEFT_ALIGN_VALUES,
                     BLOCK_TOP_LEFT_X_START_SYMBOLS+30,
                     BLOCK_TOP_LEFT_Y_START_VALUE_3 + 3);
if (!value) return;
lv_obj_add_style(value, &ui->font.very_small_20, 0);
lv_obj_set_width(value, BLOCK_TOP_LEFT_WIDTH - 20);
lv_label_set_long_mode(value, LV_LABEL_LONG_WRAP);
ui->sensor.ventilation_label = value;

#if CONFIG_HAS_SCD41
  icon = create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                     BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                     BLOCK_TOP_LEFT_X_START_SYMBOLS,
                     BLOCK_TOP_LEFT_Y_START_SYMBOLS_4, MY_TVOC_SYMBOL, ui);
  if (!icon) return;
  ui->sensor.icon_tvoc = icon;

  value = create_label(ui->sensor.screen, "--- ppb", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES,
                       BLOCK_TOP_LEFT_Y_START_VALUE_4 + 3);
  if (!value) return;
 lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.tvoc_label = value;

  // иконка прогрева SGP30 — справа от значения TVOC
  lv_obj_t *calib_icon = create_label(
      ui->sensor.screen, LV_SYMBOL_WARNING,
      BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES + 120,
      BLOCK_TOP_LEFT_Y_START_VALUE_4 + 3);
  lv_obj_set_style_text_color(calib_icon, lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_set_style_text_font(calib_icon, &lv_font_montserrat_20, 0);
  ui->sensor.tvoc_calib_icon = calib_icon;
#endif
}

void update_block_top_left(ui_main_menu_t *ui) {
    bool sht41_ok = (sht41_data.life == SHT41_STATE_OK);
    bool sgp30_ok = (sgp30_data.state == SGP30_STATE_OK);
    bool wifi_ok  = (get_wifi_status() == WIFI_CONNECTED);
#if CONFIG_HAS_SCD41
    bool scd41_ok = (scd41_data.life == SCD41_STATE_OK);
#endif

    // ── температура ───────────────────────────────────────
    if (sht41_ok) {
        float temp = get_temperature_sht41();
        if (temp > -40.0f && temp < 85.0f) {
            print_value("%.1f°C", temp, ui->sensor.temperature_label);
        } else {
            lv_label_set_text(ui->sensor.temperature_label, "Err");
        }
    } else {
        lv_label_set_text(ui->sensor.temperature_label, "K.S");
    }

    // ── влажность ─────────────────────────────────────────
    if (sht41_ok) {
        float hum = get_humidity_sht41();
        if (hum >= 0.0f && hum <= 100.0f) {
            print_value("%.0f%%", hum, ui->sensor.humidity_label);
        } else {
            lv_label_set_text(ui->sensor.humidity_label, "Err");
        }
    } else {
        lv_label_set_text(ui->sensor.humidity_label, "K.S");
    }

    // ── TVOC (nur SCD41-Variante) ──────────────────────────
#if CONFIG_HAS_SCD41
    if (sgp30_ok) {
        snprintf(ui->string_buffer, sizeof(ui->string_buffer),
                 "%u ppb", get_tvoc_sgp30());
        lv_label_set_text(ui->sensor.tvoc_label, ui->string_buffer);
    } else {
        lv_label_set_text(ui->sensor.tvoc_label, "K.S");
    }
#endif

    // ── Lueftungsempfehlung ────────────────────────────────
    const char *advice = NULL;

    // TVOC-Pruefung (beide Varianten)
    if (sgp30_ok) {
        uint16_t tvoc = get_tvoc_sgp30();
        if (tvoc > 350) {
            advice = "Lueften empfohlen";
        }
    }

#if CONFIG_HAS_SCD41
    // CO2-Pruefung (hat Vorrang vor Feuchte, aber nicht vor TVOC)
    if (advice == NULL && scd41_ok) {
        uint16_t co2 = get_co2_scd41();
        if (co2 >= 2000) {
            advice = "Lueften empfohlen!";  // HOCH — dringend
        } else if (co2 >= 1000) {
            advice = "Lueften empfohlen";   // MITTEL
        }
    }
#endif

    // Feuchte-Pruefung
    if (advice == NULL && sht41_ok) {
        float hum_indoor = get_humidity_sht41();
        if (hum_indoor > 65.0f) {
            if (wifi_ok && (float)get_weather_humidity() > hum_indoor) {
                advice = "Heizen empfohlen";
            } else {
                advice = "Lueften empfohlen";
            }
        }
    }

    // ── Label setzen ──────────────────────────────────────
    if (advice != NULL) {
        lv_label_set_text(ui->sensor.ventilation_label, advice);
        lv_obj_set_style_text_color(ui->sensor.ventilation_label,
                                    lv_palette_main(LV_PALETTE_ORANGE), 0);
    } else {
#if CONFIG_HAS_SCD41
        bool sensor_ready = scd41_ok;
#else
        bool sensor_ready = sgp30_ok;
#endif
        if (sensor_ready) {
            lv_label_set_text(ui->sensor.ventilation_label, "Luftqualitaet: GUT");
            lv_obj_set_style_text_color(ui->sensor.ventilation_label,
                                        lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_label_set_text(ui->sensor.ventilation_label, "Sensor nicht bereit");
            lv_obj_set_style_text_color(ui->sensor.ventilation_label,
                                        lv_color_hex(0x888888), 0);
        }
    }
    set_visible(ui->sensor.ventilation_label, true);

    // ── Historie ──────────────────────────────────────────
    if (sht41_ok) {
        sensor_temperature_record_values(ui);
    }
}

void ui_create_sensor_temperature_history_popup(ui_main_menu_t *ui) {
  // --- Фон popup ---
  int16_t popup_width = LVGL_PORT_H_RES - 10;
  int16_t popup_height = LVGL_PORT_V_RES - 10;

  ui->sensor.popup.popup = create_background(
      ui->screen, popup_width, popup_height, POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_add_style(ui->sensor.popup.popup, &ui->style.popup, 0);
  lv_obj_set_scrollbar_mode(ui->sensor.popup.popup, LV_SCROLLBAR_MODE_OFF);
  // --- Заголовок ---
  create_text("Temperatur / Feuchtigkeit (12 std)", ui->sensor.popup.popup,
              STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 5, ui);

  // --- Кнопка закрытия ---
  ui->sensor.popup.btn_close =
      create_btn_cb(ui->sensor.popup.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -5, -5,
                    btn_sensor_close_history_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->sensor.popup.btn_close, LV_SYMBOL_HOME, 0);

  // --- График ---
  int16_t chart_width = popup_width - 160;
  int16_t chart_height = popup_height - 125;

  lv_obj_t *chart = lv_chart_create(ui->sensor.popup.popup);
  lv_obj_set_size(chart, chart_width, chart_height);
  lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 10);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, SENSOR_HISTORY_POINTS);

  lv_obj_t *lbl_temp = lv_label_create(ui->sensor.popup.popup);
  lv_label_set_text(lbl_temp, "°C");
  lv_obj_set_style_text_color(lbl_temp, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_32, 0);
  lv_obj_align_to(lbl_temp, chart, LV_ALIGN_OUT_LEFT_TOP, -10, -35);

  // Лейбл "%" справа от графика — синий
  lv_obj_t *lbl_hum = lv_label_create(ui->sensor.popup.popup);
  lv_label_set_text(lbl_hum, "%");
  lv_obj_set_style_text_color(lbl_hum, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_text_font(lbl_hum, &lv_font_montserrat_32, 0);
  lv_obj_align_to(lbl_hum, chart, LV_ALIGN_OUT_RIGHT_TOP, 15, -35);

  // Сетка
  lv_chart_set_div_line_count(chart, 6, 12);

  // Диапазоны осей
  int16_t temperatur_range_min = 10;
  int16_t temperatur_range_max = 35;
  uint8_t humidity_range_min = 20;
  uint8_t humidity_range_max = 70;
  // Ось Y левая  — температура: 10..35 °C (×10 → 100..350)
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, temperatur_range_min,
                     temperatur_range_max);
  // Ось Y правая — влажность: 20..80 %
  lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, humidity_range_min,
                     humidity_range_max);

  // Засечки осей
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 5,
                         3,    // major/minor tick length
                         6, 1, // 6 делений, каждая подписана
                         true, 50);
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, 5, 3, 6, 1, true,
                         40);

  // --- Серия температуры (левая ось, красная) ---
  lv_chart_series_t *ser_temp = lv_chart_add_series(
      chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  // --- Серия влажности (правая ось, синяя) ---
  lv_chart_series_t *ser_hum = lv_chart_add_series(
      chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);

  // После создания серий — убрать точки совсем, оставить только линию
  lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // точки = 0px

  // Толщина линии тонче
  lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);

  // --- Заполняем серии из кольцевого буфера ---
  uint16_t n = ui->sensor.popup.history.count;

  if (n == 0) {
    for (uint16_t i = 0; i < SENSOR_HISTORY_POINTS; i++) {
      lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(chart, ser_hum, i, LV_CHART_POINT_NONE);
    }
  } else {
    uint16_t empty_slots = SENSOR_HISTORY_POINTS - n;
    for (uint16_t i = 0; i < empty_slots; i++) {
      lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(chart, ser_hum, i, LV_CHART_POINT_NONE);
    }
    for (uint16_t i = 0; i < n; i++) {
      int16_t temp_x10;
      uint8_t hum;
      sensor_temperature_history_get(&ui->sensor.popup.history, i, &temp_x10,
                                     &hum);
      int16_t current_temperaure = temp_x10;
      if (current_temperaure < temperatur_range_min * 10)
        current_temperaure = temperatur_range_min * 10;
      if (current_temperaure > temperatur_range_max * 10)
        current_temperaure = temperatur_range_max * 10;

      uint8_t current_humidity = hum;
      if (current_humidity < humidity_range_min)
        current_humidity = humidity_range_min;
      if (current_humidity > humidity_range_max)
        current_humidity = humidity_range_max;

      lv_chart_set_value_by_id(chart, ser_temp, empty_slots + i,
                               current_temperaure / 10);
      lv_chart_set_value_by_id(chart, ser_hum, empty_slots + i,
                               current_humidity);
    }
  }

  lv_chart_refresh(chart);
  ui->sensor.popup.chart = chart;
}

void block_top_left_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (ui->sensor.popup.popup != NULL && lv_obj_is_valid(ui->sensor.popup.popup))
    return;

  hide_all_blocks(ui);
  ui_create_sensor_temperature_history_popup(ui);
}

void btn_sensor_close_history_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->sensor.popup.popup);

  ui->sensor.popup.popup = NULL;
  ui->sensor.popup.btn_close = NULL;
  ui->sensor.popup.chart = NULL;

  show_all_blocks(ui);
}

void sensor_temperature_history_push(sensor_history_t *h) {
  float t = get_temperature_sht41();
  h->temperature[h->head] =
      (int16_t)(t >= 0 ? (t * 10.0f + 0.5f) : (t * 10.0f - 0.5f));
  h->humidity[h->head] = get_humidity_sht41();

  h->head = (h->head + 1) % SENSOR_HISTORY_POINTS;

  if (h->count < SENSOR_HISTORY_POINTS)
    h->count++;
}

void sensor_temperature_record_values(ui_main_menu_t *ui) {
  static uint16_t cnt_sensor_history = 0;
  cnt_sensor_history++;
  if (cnt_sensor_history >= SENSOR_RECORD_INTERVAL) {
    sensor_temperature_history_push(&ui->sensor.popup.history);
    cnt_sensor_history = 0;
  }
}

void sensor_temperature_history_get(const sensor_history_t *h, uint16_t idx,
                                    int16_t *temp_x10, uint8_t *hum) {
  uint16_t real_idx = (h->head - h->count + idx + SENSOR_HISTORY_POINTS) %
                      SENSOR_HISTORY_POINTS;
  *temp_x10 = h->temperature[real_idx];
  *hum = h->humidity[real_idx];
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Top Mid — Sensor CO2
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_top_middle(ui_main_menu_t *ui) {
  /*BLOCK TOP MID*/
  ui->co2.meter = create_meter(
      ui->screen, BLOCK_TOP_MID_WIDTH_CO2_METER, BLOCK_TOP_MID_HEIGHT_CO2_METER,
      BLOCK_TOP_MID_ALIGN_CO2_CHART, BLOCK_TOP_MID_X_START,
      BLOCK_TOP_MID_Y_START, ui);
  if (!ui->co2.meter)
    return;
  lv_obj_add_style(ui->co2.meter, &ui->style.meter_co2, 0);
  lv_meter_set_indicator_value(ui->co2.meter, ui->co2.indicator, 0);

  // --- значение CO₂ ---
  lv_obj_t *value =
      create_label(ui->screen, "0", BLOCK_TOP_MID_ALIGN_CO2_CHART,
                   BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE);
  if (!value)
    return;
  ui->co2.co2_label = value;
  lv_obj_add_style(ui->co2.co2_label, &ui->font.medium_32, 0);
  lv_obj_set_style_text_align(ui->co2.co2_label, LV_TEXT_ALIGN_CENTER, 0);

  // --- "ppb" под значением ---
  ui->co2.co2_unit_label =
      create_label(ui->screen, AQ_UNIT_STR, BLOCK_TOP_MID_ALIGN_CO2_CHART,
                   BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE - 23);
  lv_obj_add_style(ui->co2.co2_unit_label, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_align(ui->co2.co2_unit_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_opa(ui->co2.co2_unit_label, LV_OPA_70, 0);

  // --- статус бейдж "GUT" ---
  ui->co2.co2_status_label =
      create_label(ui->screen, "GUT", BLOCK_TOP_MID_ALIGN_CO2_CHART,
                   BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE + 37);
  if (!ui->co2.co2_status_label)
    return;
  lv_obj_add_style(ui->co2.co2_status_label, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_align(ui->co2.co2_status_label, LV_TEXT_ALIGN_CENTER,
                              0);
  lv_obj_set_style_text_color(ui->co2.co2_status_label,
                              lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_bg_opa(ui->co2.co2_status_label, LV_OPA_20, 0);
  lv_obj_set_style_bg_color(ui->co2.co2_status_label,
                            lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_radius(ui->co2.co2_status_label, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_hor(ui->co2.co2_status_label, 10, 0);
  lv_obj_set_style_pad_ver(ui->co2.co2_status_label, 3, 0);
  lv_obj_set_style_border_width(ui->co2.co2_status_label, 1, 0);
  lv_obj_set_style_border_color(ui->co2.co2_status_label,
                                lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_obj_set_style_border_opa(ui->co2.co2_status_label, LV_OPA_50, 0);

  // --- футер "CO₂ | Air Quality" ---
  ui->co2.co2_footer_label = create_label(
      ui->screen, AQ_FOOTER_STR, BLOCK_TOP_MID_ALIGN_CO2_CHART,
      BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE - 60);
  lv_obj_set_style_text_font(ui->co2.co2_footer_label, &lv_font_montserrat_12,
                             0);
  lv_obj_set_style_text_align(ui->co2.co2_footer_label, LV_TEXT_ALIGN_CENTER,
                              0);
  lv_obj_add_style(ui->co2.co2_footer_label, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_align(ui->co2.co2_footer_label, LV_TEXT_ALIGN_CENTER,
                              0);

  // центральный кружок поверх иглы
  lv_obj_t *center_dot = lv_obj_create(ui->screen);
  lv_obj_remove_style_all(center_dot);
  lv_obj_set_size(center_dot, 14, 14);
  lv_obj_set_style_radius(center_dot, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_opa(center_dot, LV_OPA_COVER, 0);
  lv_obj_set_style_bg_color(center_dot, lv_color_hex(0x888888), 0);
  lv_obj_set_style_border_width(center_dot, 2, 0);
  lv_obj_set_style_border_color(center_dot, lv_color_hex(0xCCCCCC), 0);
  lv_obj_align_to(center_dot, ui->co2.meter, LV_ALIGN_CENTER, 0, 0);

  // --- иконка калибровки ---
 ui->co2.calib_icon_label =
      create_label(ui->screen, LV_SYMBOL_WARNING, BLOCK_TOP_MID_ALIGN_CO2_CHART,
                   0, BLOCK_TOP_MID_Y_START + 40);
  lv_obj_set_style_text_color(ui->co2.calib_icon_label,
                              lv_palette_main(LV_PALETTE_ORANGE), 0);
  lv_obj_set_style_text_font(ui->co2.calib_icon_label, &lv_font_montserrat_20,
                             0);
#if CONFIG_HAS_SCD41
  lv_obj_add_flag(ui->co2.calib_icon_label, LV_OBJ_FLAG_HIDDEN);
#endif

  // --- метр кликабельный ---
  lv_obj_add_flag(ui->co2.meter, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(ui->co2.meter, co2_meter_click_event_cb, LV_EVENT_CLICKED,
                      ui);
  /*BLOCK TOP MID*/
}

void update_block_top_middle(ui_main_menu_t *ui) {
    if (AQ_SENSOR_OK()) {
        uint16_t raw = AQ_GET_VALUE();
        ui->co2.co2_target = (int32_t)constrain(raw, MIN_VALUE_CO2, MAX_VALUE_CO2);
        print_value("%.f", (float)raw, ui->co2.co2_label);
        update_co2_status_label(ui, raw);
    } else {
        lv_label_set_text(ui->co2.co2_label, "K.S");
        lv_label_set_text(ui->co2.co2_status_label, "---");
    }
}

void co2_calib_popup_close_cb(lv_event_t *e) {
  ui_main_menu_t *ui = lv_event_get_user_data(e);
  lv_obj_add_flag(ui->co2.calib_popup, LV_OBJ_FLAG_HIDDEN);
}

void co2_meter_click_event_cb(lv_event_t *e) {
  #if CONFIG_HAS_SCD41
  ui_main_menu_t *ui = lv_event_get_user_data(e);
  lv_label_set_text(ui->co2.calib_popup_text,
                    "Echtzeit-CO2-Messung per SCD41.\n\n"
                    "Gut: < 1000 / Mittel: 1000-2000 / Hoch: > 2000 ppm\n");
  lv_obj_clear_flag(ui->co2.calib_popup, LV_OBJ_FLAG_HIDDEN);
#else
  if (sgp30_data.init_ok) {
    ui_main_menu_t *ui = lv_event_get_user_data(e);

    TickType_t elapsed = xTaskGetTickCount() - ui->co2.calib_start_tick;
    TickType_t total = CO2_CALIB_TIME;

    if (elapsed < total) {
      uint32_t remaining_sec = (total - elapsed) / configTICK_RATE_HZ;
      uint32_t h = remaining_sec / 3600;
      uint32_t m = (remaining_sec % 3600) / 60;
      lv_label_set_text_fmt(ui->co2.calib_popup_text,
                            "Der TVOC-Sensor befindet sich in der Lernphase.\n"
                            "Die Werte sind noch ungenau.\n\n"
                            "Verbleibende Zeit: %02u:%02u Std.",
                            (unsigned int)h, (unsigned int)m);
    } else {
      lv_label_set_text(ui->co2.calib_popup_text,
                        "Der TVOC-Sensor ist kalibriert.\n"
                        "Die angezeigten Werte sind zuverlaessig.");
    }

    lv_obj_clear_flag(ui->co2.calib_popup, LV_OBJ_FLAG_HIDDEN);
  } else
    return;
#endif
}

void ui_create_sgp30_calib_popup(ui_main_menu_t *ui) {
  ui->co2.calib_popup = create_background(ui->screen, POPUP_WINDOW_WIDTH, 220,
                                          POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_add_style(ui->co2.calib_popup, &ui->style.popup, 0);
  lv_obj_add_flag(ui->co2.calib_popup, LV_OBJ_FLAG_HIDDEN);

  ui->co2.calib_popup_text =
      create_label(ui->co2.calib_popup,
                   "Der TVOC-Sensor befindet sich in der Lernphase.\n"
                   "Die Werte sind noch ungenau.\n"
                   "Die Genauigkeit verbessert sich nach ~12 Stunden.",
                   LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_set_style_text_align(ui->co2.calib_popup_text, LV_TEXT_ALIGN_CENTER,
                              0);
  lv_obj_add_style(ui->co2.calib_popup_text, &ui->font.very_small_20, 0);
  lv_obj_set_width(ui->co2.calib_popup_text, POPUP_WINDOW_WIDTH - 40);

  lv_obj_t *btn =
      create_btn_cb(ui->co2.calib_popup, 120, 45, LV_ALIGN_BOTTOM_MID, 0, -15,
                    co2_calib_popup_close_cb, ui);
  lv_obj_t *lbl = lv_label_create(btn);
  lv_label_set_text(lbl, "OK");
  lv_obj_center(lbl);
}


lv_color_t calc_co2_color(uint16_t val) {
#if CONFIG_HAS_SCD41
    // CO2: 400..2400 ppm  (UBA-Standard: GUT <1000, MITTEL 1000-2000, HOCH >2000)
    if (val < 1000) return lv_palette_main(LV_PALETTE_GREEN);
    if (val < 1300) return lv_color_hex(0xC6FF00);
    if (val < 1600) return lv_palette_main(LV_PALETTE_YELLOW);
    if (val < 2000) return lv_palette_main(LV_PALETTE_ORANGE);
    return lv_palette_main(LV_PALETTE_RED);
#else
    // TVOC: 0..1000 ppb (unveraendert)
    if (val < 150) return lv_palette_main(LV_PALETTE_GREEN);
    if (val < 350) return lv_color_hex(0xC6FF00);
    if (val < 600) return lv_palette_main(LV_PALETTE_YELLOW);
    if (val < 800) return lv_palette_main(LV_PALETTE_ORANGE);
    return lv_palette_main(LV_PALETTE_RED);
#endif
}

void update_co2_status_label(ui_main_menu_t *ui, uint16_t co2_ppm) {
  if (!ui->co2.co2_status_label)
    return;

  const char *text;
  lv_color_t color;

   if (co2_ppm < AQ_THRESH_GUT) {
    text = "GUT";
    color = lv_palette_main(LV_PALETTE_GREEN);
  } else if (co2_ppm < AQ_THRESH_MITTEL) {
    text = "MITTEL";
    color = lv_palette_main(LV_PALETTE_YELLOW);
  } else {
    text = "HOCH";
    color = lv_palette_main(LV_PALETTE_RED);
  }

  lv_label_set_text(ui->co2.co2_status_label, text);
  lv_obj_set_style_text_color(ui->co2.co2_status_label, color, 0);
  lv_obj_set_style_bg_color(ui->co2.co2_status_label, color, 0);
  lv_obj_set_style_border_color(ui->co2.co2_status_label, color, 0);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Top Right — Time & Date
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_top_right(ui_main_menu_t *ui) {
  /*BLOCK TOP RIGHT*/
  lv_obj_t *bg = create_background(
      ui->screen, BLOCK_TOP_RIGHT_WIDTH, BLOCK_TOP_RIGHT_HEIGHT,
      BLOCK_TOP_RIGHT_ALIGN_BACKGROUND, BLOCK_TOP_RIGHT_X_START,
      BLOCK_TOP_RIGHT_Y_START);
  if (!bg)
    return;
  ui->time.screen = bg;
  lv_obj_add_style(ui->time.screen, &ui->style.top_right, 0);
  lv_obj_set_scrollbar_mode(ui->time.screen, LV_SCROLLBAR_MODE_OFF);

  // --- время HH:MM ---
  lv_obj_t *lbl = create_label(
      ui->time.screen, "00:00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
      BLOCK_TOP_RIGHT_X_START_VALUES, BLOCK_TOP_RIGHT_Y_START_VALUE_1);
  if (!lbl)
    return;
  ui->time.hour_minute_label = lbl;
  lv_obj_add_style(ui->time.hour_minute_label, &ui->font.time, 0);

  // --- дата DD.MM ---
  lbl = create_label(ui->time.screen, "00.00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
                     BLOCK_TOP_RIGHT_X_START_VALUES_2,
                     BLOCK_TOP_RIGHT_Y_START_VALUE_2);
  if (!lbl)
    return;
  ui->time.mday_month_label = lbl;
  lv_obj_add_style(ui->time.mday_month_label, &ui->font.time, 0);

  // --- колонка дней недели (Mo Di Mi Do Fr Sa So) ---
  static const char *wday_names[7] = {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};

  for (int i = 0; i < 7; i++) {
    lv_coord_t y = BLOCK_TOP_RIGHT_WDAY_COL_Y_START +
                   i * (BLOCK_TOP_RIGHT_WDAY_COL_ITEM_HEIGHT +
                        BLOCK_TOP_RIGHT_WDAY_COL_ITEM_GAP);
    lbl = create_label(ui->time.screen, wday_names[i],
                       BLOCK_TOP_RIGHT_WDAY_COL_ALIGN,
                       BLOCK_TOP_RIGHT_WDAY_COL_X_START, y);
    if (!lbl)
      return;
    lv_obj_add_style(lbl, &ui->font.small_24, 0);
    lv_obj_set_width(lbl, BLOCK_TOP_RIGHT_WDAY_COL_WIDTH);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    ui->time.wday_labels[i] = lbl;
  }

  // --- highlight прямоугольник за текущим днём ---
  lv_obj_t *hl = lv_obj_create(ui->time.screen);
  lv_obj_remove_style_all(hl);
  lv_obj_set_size(hl, BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_WIDTH,
                  BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_HEIGHT);
  lv_obj_set_style_radius(hl, BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_RADIUS, 0);
  lv_obj_set_style_border_width(hl, 2, 0);
  lv_obj_set_style_border_color(hl, lv_color_hex(0x5E4E90), 0);
  lv_obj_set_style_bg_opa(hl, LV_OPA_TRANSP, 0);
  lv_obj_align_to(hl, ui->time.wday_labels[0], LV_ALIGN_CENTER, 0, 0);
  ui->time.wday_highlight = hl;

  // --- строка праздника внизу ---
  lbl = create_label(ui->time.screen, "", BLOCK_TOP_RIGHT_HOLIDAY_ALIGN, -28,
                     BLOCK_TOP_RIGHT_HOLIDAY_Y_START);
  if (!lbl)
    return;
  lv_obj_add_style(lbl, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN);
  ui->time.holiday_label = lbl;
  /*BLOCK TOP RIGHT*/
}

void update_block_top_right(ui_main_menu_t *ui) {
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    print_mday(0, 0, ui);
    print_wday(WDAY_KEIN_WLAN, ui);
    print_time(0, 0, ui);
    lv_obj_add_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    print_mday(get_time_mday(), get_time_month(), ui);
    print_wday(get_time_wday(), ui);
    print_time(get_time_hour(), get_time_minute(), ui);
    print_holiday(get_time_mday(), get_time_month(), get_time_year(), ui);
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Bot Left — Weather Open Meteo
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_bot_left(ui_main_menu_t *ui) {
  /*BLOCK BOT LEFT*/
  lv_obj_t *bg =
      create_background(ui->screen, BLOCK_BOT_LEFT_WIDTH, BLOCK_BOT_LEFT_HEIGHT,
                        BLOCK_BOT_LEFT_ALIGN_BACKGROUND, BLOCK_BOT_LEFT_X_START,
                        BLOCK_BOT_LEFT_Y_START);
  if (!bg)
    return;

  ui->weather.screen = bg;
  lv_obj_add_style(ui->weather.screen, &ui->style.bot_left, 0);
  lv_obj_set_scrollbar_mode(ui->weather.screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(ui->weather.screen,
                      block_bot_left_open_popup_event_handler, LV_EVENT_CLICKED,
                      ui);

  create_text("aussen", ui->weather.screen, STYLE_TEXT_SMALL,
              BLOCK_BOT_LEFT_ALIGN_TITLE, 0, BLOCK_BOT_LEFT_Y_START_TITLE, ui);

  // ═══════════════════════════════════════════
  // СТРОКА 1 — температура (лево) + влажность (право)
  // ═══════════════════════════════════════════
  lv_obj_t *icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_1, MY_TEMPERATURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_temperature = icon;

  lv_obj_t *value = create_label(
      ui->weather.screen, "0°C", BLOCK_BOT_LEFT_ALIGN_VALUES,
      BLOCK_BOT_LEFT_X_START_VALUES, BLOCK_BOT_LEFT_Y_START_VALUE_1);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.temperature_label = value;

  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_1, MY_HUMIDITY_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_humidity = icon;

  value = create_label(ui->weather.screen, "0%", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_1);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.humidity_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 2 — feels like (лево) + UV (право)
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_2, MY_FEELS_LIKE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_feels_like = icon;

  value = create_label(ui->weather.screen, "0°C", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_2);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.feels_like_label = value;

  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_2, MY_SUN_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_uv = icon;

  value = create_label(ui->weather.screen, "UV 0", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_2);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.uv_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 3 — осадки
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_3, MY_CLOUD_RAIN_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_rain = icon;

  value = create_label(ui->weather.screen, "---", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.rain_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 4 — ветер (лево) + давление (право)
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_4, MY_WIND_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_wind = icon;

  value = create_label(ui->weather.screen, "0m/s", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_4);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.wind_label = value;

  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_4, MY_PRESSURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_pressure = icon;

  value = create_label(ui->weather.screen, "0hPa", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_4);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.pressure_label = value;
  /*BLOCK BOT LEFT*/
}

void update_block_bot_left(ui_main_menu_t *ui) {
  bool wifi_ok = (get_wifi_status() == WIFI_CONNECTED);

  if (!wifi_ok) {
    lv_label_set_text(ui->weather.temperature_label, "---");
    lv_label_set_text(ui->weather.humidity_label, "---");
    lv_label_set_text(ui->weather.feels_like_label, "---");
    lv_label_set_text(ui->weather.uv_label, "---");
    lv_label_set_text(ui->weather.rain_label, "---");
    lv_label_set_text(ui->weather.wind_label, "---");
    lv_label_set_text(ui->weather.pressure_label, "---");
    return;
  }

  float temp = get_weather_temperature();
  float hum = (float)get_weather_humidity();

  if (temp > -60.0f && temp < 60.0f) {
    print_value("%.1f°C", temp, ui->weather.temperature_label);
  } else {
    lv_label_set_text(ui->weather.temperature_label, "Err");
  }

  if (hum >= 0.0f && hum <= 100.0f) {
    print_value("%.0f%%", hum, ui->weather.humidity_label);
  } else {
    lv_label_set_text(ui->weather.humidity_label, "Err");
  }

  float feels = (float)get_apparent_temperature();
  if (feels > -60.0f && feels < 60.0f) {
    print_value("~%.1f°C", feels, ui->weather.feels_like_label);
  } else {
    lv_label_set_text(ui->weather.feels_like_label, "Err");
  }

  float uv = (float)get_uv_index();
  if (uv >= 0.0f && uv <= 20.0f) {
    snprintf(ui->string_buffer, sizeof(ui->string_buffer), "%.0f", uv);
    lv_label_set_text(ui->weather.uv_label, ui->string_buffer);
    lv_obj_set_style_text_color(ui->weather.uv_label, calc_uv_color(uv), 0);
  } else {
    lv_label_set_text(ui->weather.uv_label, "UV -");
    lv_obj_remove_local_style_prop(ui->weather.uv_label, LV_STYLE_TEXT_COLOR,
                                   0);
  }

  float rain_prob = (float)get_precipitation_probability();
  const char *precip_text;
  if (rain_prob < 30.0f) {
    precip_text = "  kaum";
  } else if (rain_prob < 60.0f) {
    precip_text = "  moeglich";
  } else if (rain_prob < 80.0f) {
    precip_text = "  wahrsch.";
  } else {
    precip_text = "  sicher";
  }
  lv_label_set_text(ui->weather.rain_label, precip_text);

  float wind = (float)get_weather_wind();
  float press = (float)get_surface_pressure();

  if (wind >= 0.0f) {
    print_value("%.0fm/s", wind, ui->weather.wind_label);
  } else {
    lv_label_set_text(ui->weather.wind_label, "- m/s");
  }

  if (press > 800.0f && press < 1100.0f) {
    print_value("%.0f", press, ui->weather.pressure_label);
  } else {
    lv_label_set_text(ui->weather.pressure_label, "- hPa");
  }
}

void block_bot_left_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (ui->weather.forecast_popup.popup != NULL &&
      lv_obj_is_valid(ui->weather.forecast_popup.popup))
    return;

  hide_all_blocks(ui);
  ui_create_weather_forecast_popup(ui);
}

void btn_weather_close_forecast_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->weather.forecast_popup.popup);
  ui->weather.forecast_popup.popup = NULL;
  ui->weather.forecast_popup.btn_close = NULL;

  show_all_blocks(ui);
}

void ui_create_weather_forecast_popup(ui_main_menu_t *ui) {
  int16_t popup_width = LVGL_PORT_H_RES - 10;
  int16_t popup_height = LVGL_PORT_V_RES - 10;

  ui->weather.forecast_popup.popup = create_background(
      ui->screen, popup_width, popup_height, POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_set_scrollbar_mode(ui->weather.forecast_popup.popup,
                            LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_style(ui->weather.forecast_popup.popup, &ui->style.popup, 0);

  create_text("Wettervorhersage (3 Tage)", ui->weather.forecast_popup.popup,
              STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 5, ui);

  ui->weather.forecast_popup.btn_close = create_btn_cb(
      ui->weather.forecast_popup.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -5, -5,
      btn_weather_close_forecast_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.forecast_popup.btn_close,
                              LV_SYMBOL_HOME, 0);

  const char *day_names[FORECAST_DAYS] = {"Heute", "Morgen", "+2 Tage"};
  int16_t col_width = (popup_width - 40) / FORECAST_DAYS;
  int16_t col_y_start = 70;
  int16_t row_h = 55;

  for (int i = 0; i < FORECAST_DAYS; i++) {
    int16_t col_x = -popup_width / 2 + 20 + i * col_width + col_width / 2;

    if (i > 0) {
      lv_obj_t *sep = lv_obj_create(ui->weather.forecast_popup.popup);
      lv_obj_remove_style_all(sep);
      lv_obj_set_size(sep, 1, popup_height - 80);
      lv_obj_set_style_bg_color(sep, lv_color_hex(0x444444), 0);
      lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
      lv_obj_align(sep, LV_ALIGN_TOP_MID, -popup_width / 2 + 20 + i * col_width,
                   col_y_start - 10);
    }

    ui->weather.forecast_popup.day_label[i] =
        create_label(ui->weather.forecast_popup.popup, day_names[i],
                     LV_ALIGN_TOP_MID, col_x, col_y_start);
    lv_obj_add_style(ui->weather.forecast_popup.day_label[i],
                     &ui->font.medium_32, 0);

    ui->weather.forecast_popup.wcode_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h);
    lv_obj_add_style(ui->weather.forecast_popup.wcode_label[i],
                     &ui->font.very_small_20, 0);

    ui->weather.forecast_popup.temp_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 2);
    lv_obj_add_style(ui->weather.forecast_popup.temp_label[i],
                     &ui->font.medium_32, 0);

    ui->weather.forecast_popup.humidity_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 3);
    lv_obj_add_style(ui->weather.forecast_popup.humidity_label[i],
                     &ui->font.very_small_20, 0);

    ui->weather.forecast_popup.sunrise_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 4);
    lv_obj_add_style(ui->weather.forecast_popup.sunrise_label[i],
                     &ui->font.very_small_20, 0);

    ui->weather.forecast_popup.sunset_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 5);
    lv_obj_add_style(ui->weather.forecast_popup.sunset_label[i],
                     &ui->font.very_small_20, 0);

    if (forecast_data.valid) {
      forecast_day_t *d = &forecast_data.day[i];
      char buf[32];

      snprintf(buf, sizeof(buf), "%d / %d °C", d->temp_min, d->temp_max);
      lv_label_set_text(ui->weather.forecast_popup.temp_label[i], buf);

      lv_label_set_text(ui->weather.forecast_popup.wcode_label[i],
                        weathercode_to_text(d->weathercode));

      snprintf(buf, sizeof(buf), "%d %%", d->humidity_max);
      lv_label_set_text(ui->weather.forecast_popup.humidity_label[i], buf);

      time_t sr = (time_t)d->sunrise;
      struct tm *sr_tm = localtime(&sr);
      snprintf(buf, sizeof(buf), LV_SYMBOL_UP " %02d:%02d", sr_tm->tm_hour,
               sr_tm->tm_min);
      lv_label_set_text(ui->weather.forecast_popup.sunrise_label[i], buf);

      time_t ss = (time_t)d->sunset;
      struct tm *ss_tm = localtime(&ss);
      snprintf(buf, sizeof(buf), LV_SYMBOL_DOWN " %02d:%02d", ss_tm->tm_hour,
               ss_tm->tm_min);
      lv_label_set_text(ui->weather.forecast_popup.sunset_label[i], buf);
    }
  }
}

lv_color_t calc_uv_color(float uv) {
  if (uv < 3.0f)
    return lv_palette_main(LV_PALETTE_GREEN);
  if (uv < 6.0f)
    return lv_color_hex(0xB3B300);
  if (uv < 8.0f)
    return lv_palette_main(LV_PALETTE_ORANGE);
  if (uv < 11.0f)
    return lv_palette_main(LV_PALETTE_RED);
  return lv_color_hex(0x8B00FF);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Bot Mid — CO2 Chart & Settings Btn
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_bot_middle(ui_main_menu_t *ui) {
  ui_create_co2_chart_bot_mid(ui);
  ui_create_settings_popup_btns(ui);
}

void update_block_bot_middle(ui_main_menu_t *ui) {
  static uint16_t cnt_chart_co2 = 0;
  cnt_chart_co2++;
  if (cnt_chart_co2 > 3600) {
  int16_t raw_chart = (int16_t)AQ_GET_VALUE();
int16_t clamped = (int16_t)constrain(raw_chart, MIN_VALUE_CO2, MAX_VALUE_CO2);
lv_chart_set_next_value(ui->co2.chart, ui->co2.series_co2, clamped);
    cnt_chart_co2 = 0;
  }
  update_symbol_wifi(ui);
}

void ui_create_co2_chart_bot_mid(ui_main_menu_t *ui) {
  ui->co2.chart = create_chart(
      ui->screen, BLOCK_BOT_MID_WIDTH_CO2_CHART, BLOCK_BOT_MID_HEIGHT_CO2_CHART,
      BLOCK_BOT_MID_ALIGN_CO2_CHART, 0, BLOCK_BOT_MID_Y_START_CO2_CHART, ui);

  lv_obj_add_style(ui->co2.chart, &ui->style.chart_co2, 0);

  ui->co2.chart_lbl_max = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_max, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_max, LV_OPA_50, 0);
  lv_obj_align_to(ui->co2.chart_lbl_max, ui->co2.chart, LV_ALIGN_TOP_LEFT, 4,
                  2);

  ui->co2.chart_lbl_mid = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_mid, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_mid, LV_OPA_50, 0);
  lv_obj_align_to(ui->co2.chart_lbl_mid, ui->co2.chart, LV_ALIGN_LEFT_MID, 4,
                  0);

  ui->co2.chart_lbl_min = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_min, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_min, LV_OPA_50, 0);
  lv_obj_align_to(ui->co2.chart_lbl_min, ui->co2.chart, LV_ALIGN_BOTTOM_LEFT, 4,
                  -2);

  ui->co2.chart_lbl_t24 = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_t24, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_t24, LV_OPA_70, 0);
  lv_label_set_text(ui->co2.chart_lbl_t24, "-24h");
  lv_obj_align_to(ui->co2.chart_lbl_t24, ui->co2.chart, LV_ALIGN_BOTTOM_LEFT, 2,
                  BLOCK_BOT_MID_Y_START_CO2_TIMES);

  ui->co2.chart_lbl_t12 = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_t12, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_t12, LV_OPA_70, 0);
  lv_label_set_text(ui->co2.chart_lbl_t12, "-12h");
  lv_obj_align_to(ui->co2.chart_lbl_t12, ui->co2.chart, LV_ALIGN_BOTTOM_MID, 0,
                  BLOCK_BOT_MID_Y_START_CO2_TIMES);

  ui->co2.chart_lbl_t0 = lv_label_create(ui->screen);
  lv_obj_add_style(ui->co2.chart_lbl_t0, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_opa(ui->co2.chart_lbl_t0, LV_OPA_70, 0);
  lv_label_set_text(ui->co2.chart_lbl_t0, "0h");
  lv_obj_align_to(ui->co2.chart_lbl_t0, ui->co2.chart, LV_ALIGN_BOTTOM_RIGHT,
                  -2, BLOCK_BOT_MID_Y_START_CO2_TIMES);

  update_co2_chart_labels(ui);
}

void update_co2_chart_labels(ui_main_menu_t *ui) {
  char buf[16];

  snprintf(buf, sizeof(buf), "%d %s", MAX_VALUE_CO2, AQ_UNIT_STR);
  lv_label_set_text(ui->co2.chart_lbl_max, buf);

  snprintf(buf, sizeof(buf), "%d %s", (MIN_VALUE_CO2 + MAX_VALUE_CO2) / 2, AQ_UNIT_STR);
  lv_label_set_text(ui->co2.chart_lbl_mid, buf);

  snprintf(buf, sizeof(buf), "%d %s", MIN_VALUE_CO2, AQ_UNIT_STR);
  lv_label_set_text(ui->co2.chart_lbl_min, buf);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Block Bot Right — Weather Animations
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void create_block_bot_right(ui_main_menu_t *ui) {
  ui->animation.screen = create_background(
      ui->screen, BLOCK_BOT_RIGHT_WIDTH, BLOCK_BOT_RIGHT_HEIGHT,
      BLOCK_BOT_RIGHT_ALIGN_BACKGROUND, BLOCK_BOT_RIGHT_X_START,
      BLOCK_BOT_RIGHT_Y_START);
  lv_obj_add_style(ui->animation.screen, &ui->style.bot_right, 0);
  lv_obj_set_scrollbar_mode(ui->animation.screen, LV_SCROLLBAR_MODE_OFF);

  create_text("wetter", ui->animation.screen, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, BLOCK_BOT_RIGHT_Y_START_TITLE, ui);

#if ACTIVATE_ANIM_SUN_MOON
  ui->animation.image.sun_48_48 = create_anim_image_orbit(
      &sun_48_48, ui->animation.screen, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);

  ui->animation.image.moon_42_42 = create_anim_image_orbit(
      &moon_42_42, ui->animation.screen, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);
#endif
  set_visible(ui->animation.image.sun_48_48, false);
  set_visible(ui->animation.image.moon_42_42, false);

#if ACTIVATE_ANIM_CLOUD
  ui->animation.image.cloud.big_110_50 = create_cloud_anim(
      &cloud_big_110_50, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_BIG_110_50);
  set_visible(ui->animation.image.cloud.big_110_50, false);

  ui->animation.image.cloud.mid_90_45 =
      create_cloud_anim(&cloud_mid_90_45, ui->animation.screen,
                        BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_MID_90_45);
  set_visible(ui->animation.image.cloud.mid_90_45, false);

  ui->animation.image.cloud.small_70_35 = create_cloud_anim(
      &cloud_small_70_35, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_SMALL_70_35);
  set_visible(ui->animation.image.cloud.small_70_35, false);

  ui->animation.image.cloud.thin_80_30 = create_cloud_anim(
      &cloud_thin_80_30, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_THIN_80_30);
  set_visible(ui->animation.image.cloud.thin_80_30, false);
#endif

#if ACTIVATE_ANIM_WIND
  ui->animation.wind.slow =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_SLOW);
  set_visible(ui->animation.wind.slow, false);

  ui->animation.wind.med =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_MED);
  set_visible(ui->animation.wind.med, false);

  ui->animation.wind.fast =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_FAST);
  set_visible(ui->animation.wind.fast, false);
#endif

#if ACTIVATE_ANIM_RAIN
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS; i++) {
    ui->animation.rain[i] =
        create_rain_anim(&rain_drop_heavy_9_22, ui->animation.screen, 2,
                         BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
    set_visible(ui->animation.rain[i], false);
  }
#endif

#if ACTIVATE_ANIM_SNOW
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS; i++) {
    ui->animation.snow[i] =
        create_snow_anim(&snow_flake_2_15_15, ui->animation.screen, 0,
                         BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
    set_visible(ui->animation.snow[i], false);
  }
#endif
}

void update_block_bot_right(ui_main_menu_t *ui) {
  static uint16_t cnt_weather = 0;
  cnt_weather++;
  if (cnt_weather < 2) {
    draw_weather(ui);
  }
  if (cnt_weather > UPDATE_WEATHER_SEC + 2) {
    draw_weather(ui);
    cnt_weather = 2;
  }
}
