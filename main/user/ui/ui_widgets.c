
#include "ui_widgets.h"
#include "ui_core.h"

void create_text(const char *text, lv_obj_t *parent, uint16_t theme,
                 lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                 ui_main_menu_t *ui) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_text");
    return;
  }
  lv_obj_t *cont = lv_label_create(parent);
  lv_obj_remove_style_all(cont);
  switch (theme) {
  case STYLE_TEXT_SMALL:

    lv_obj_add_style(cont, &ui->font.medium_32, 0);
    break;
  case STYLE_TEXT_TITLE:
    lv_obj_add_style(cont, &ui->font.very_small_20, 0);
    break;
  }
  lv_label_set_text(cont, text);
  lv_obj_align(cont, align, x_ofs, y_ofs);
}

lv_obj_t *create_label(lv_obj_t *parent, const char *text, lv_align_t align,
                       lv_coord_t x_ofs, lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_label");
    return NULL;
  }

  lv_obj_t *con = lv_label_create(parent);
  lv_obj_align(con, align, x_ofs, y_ofs);
  lv_label_set_text(con, text);
  return con;
}

lv_obj_t *create_meter(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                       lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                       ui_main_menu_t *ui) {
  const int16_t start_value = BLOCK_TOP_MID_START_CO2_LEFT_PART;
  const int16_t start_value_1 = BLOCK_TOP_MID_END_CO2_LEFT_PART;
  const int16_t end_value = BLOCK_TOP_MID_START_CO2_RIGHT_PART;
  const int16_t end_value_1 = BLOCK_TOP_MID_END_CO2_RIGHT_PART;

  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_meter");
    return NULL;
  }

  lv_obj_t *meter = lv_meter_create(parent);
  lv_obj_set_size(meter, w, h);
  lv_obj_align(meter, align, x_ofs, y_ofs);

  /*Add a scale first*/

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_range(meter, scale, MIN_VALUE_CO2, MAX_VALUE_CO2, 250,
                           145);

  // мелкие тики — много, тонкие, короткие
  lv_meter_set_scale_ticks(meter, scale, 41, 2, 8,
                           lv_palette_main(LV_PALETTE_GREY));

  // major тики — поверх мелких, толстые и длинные
  lv_meter_scale_t *scale_major = lv_meter_add_scale(meter);
  lv_meter_set_scale_range(meter, scale_major, MIN_VALUE_CO2, MAX_VALUE_CO2,
                           250, 145);
  lv_meter_set_scale_ticks(meter, scale_major, 6, 4, 18,
                           lv_color_hex(0xAAAAAA));

  /*Add a blue arc to the start*/
  ui->co2.indicator =
      lv_meter_add_arc(meter, scale, 4, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, start_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, start_value_1);

  /*Make the tick lines blue at the start of the scale*/
  ui->co2.indicator =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_GREEN),
                               lv_palette_main(LV_PALETTE_GREEN), false, 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, start_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, start_value_1);

  /*Add a red arc to the end*/
  ui->co2.indicator =
      lv_meter_add_arc(meter, scale, 4, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  /*Make the tick lines red at the end of the scale*/
  ui->co2.indicator =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED),
                               lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  ui->co2.needle_arc =
      lv_meter_add_arc(meter, scale, 12, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.needle_arc, MIN_VALUE_CO2);
  lv_meter_set_indicator_end_value(meter, ui->co2.needle_arc, MIN_VALUE_CO2);

  /*Add a needle line indicator*/
  ui->co2.indicator =
      lv_meter_add_needle_line(meter, scale, 2, lv_color_hex(0xAAAAAA), -15);

  return meter;
}

lv_obj_t *create_chart(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                       lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                       ui_main_menu_t *ui) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_meter");
    return NULL;
  }

  lv_obj_t *chart = lv_chart_create(parent);
  lv_obj_set_size(chart, w, h);
  lv_obj_align(chart, align, x_ofs, y_ofs);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, 24); // 24 точки = 24 часа
  lv_chart_set_div_line_count(chart, 3, 6);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, MIN_VALUE_CO2,
                     MAX_VALUE_CO2);

  ui->co2.series_co2 = lv_chart_add_series(chart, lv_color_hex(0x2E86C1),
                                           LV_CHART_AXIS_PRIMARY_Y);

  return chart;
}

lv_obj_t *create_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                      lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                      const char *symbol, ui_main_menu_t *ui) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_icon");
    return NULL;
  }
  lv_obj_t *cont =
      lv_obj_create(parent); // lv_obj вместо lv_btn — нет дефолтных стилей
  lv_obj_remove_style_all(cont);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, symbol);
  lv_obj_add_style(label, &ui->font.icon, 0);
  lv_obj_center(label);

  return cont;
}
lv_obj_t *create_btn_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                          lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                          lv_event_cb_t event_cb, void *user_data,
                          const char *symbol, lv_style_t *icon_style,
                          const lv_font_t *font, const char *label_text,
                          ui_main_menu_t *ui_ptr) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_btn_icon");
    return NULL;
  }

  lv_obj_t *cont = lv_btn_create(parent);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_CLICKED, user_data);

  /* фон из style.popup — переключается с темой автоматически */
  lv_obj_add_style(cont, &ui_ptr->style.popup, 0);
  lv_obj_set_style_radius(cont, 10, 0);
  lv_obj_set_style_border_width(cont, 1, 0);
  lv_obj_set_style_border_color(cont, lv_color_hex(0x2D5A8E), 0);
  lv_obj_set_style_border_opa(cont, LV_OPA_60, 0);
  lv_obj_set_style_shadow_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 0, 0);

  /* pressed */
  lv_obj_set_style_bg_opa(cont, LV_OPA_COVER, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x2D6AB4), LV_STATE_PRESSED);
  lv_obj_set_style_border_color(cont, lv_color_hex(0x60A5FA), LV_STATE_PRESSED);

  /* иконка */
  lv_obj_t *icon = lv_label_create(cont);
  lv_label_set_text(icon, symbol);
  lv_obj_add_style(icon, &ui_ptr->font.nav_btn, 0);
  if (font)
    lv_obj_set_style_text_font(icon, font, 0);
  lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 6);

  /* подпись */
  lv_obj_t *lbl = lv_label_create(cont);
  lv_label_set_text(lbl, label_text);
  lv_obj_add_style(lbl, &ui_ptr->font.nav_btn, 0);
  lv_obj_set_style_text_font(lbl, &lv_font_montserrat_12, 0);
  lv_obj_align(lbl, LV_ALIGN_BOTTOM_MID, 0, -5);

  return cont;
}

lv_obj_t *create_btn_cb(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                        lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                        lv_event_cb_t event_cb, void *user_data) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_btn_cb");
    // return;
  }
  lv_obj_t *cont = lv_btn_create(parent);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_CLICKED, user_data);

  return cont;
}

lv_obj_t *create_background(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                            lv_align_t align, lv_coord_t x_ofs,
                            lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_button");
    // return;
  }
  lv_obj_t *cont = lv_obj_create(parent);
  if (cont == NULL)
    return NULL;
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  return cont;
}