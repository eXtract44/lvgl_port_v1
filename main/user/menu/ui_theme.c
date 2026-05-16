#include "ui_theme.h"
#include "ui_core.h"

extern lv_font_t my_symbols;
extern lv_font_t my_time_font;
//extern ui_main_menu_t ui;

void init_fonts(ui_main_menu_t *ui) {
  lv_style_init(&ui->font.very_small_20);
  lv_style_set_text_font(&ui->font.very_small_20, &lv_font_montserrat_20);

  lv_style_init(&ui->font.small_24);
  lv_style_set_text_font(&ui->font.small_24, &lv_font_montserrat_24);

  lv_style_init(&ui->font.medium_32);
  lv_style_set_text_font(&ui->font.medium_32, &lv_font_montserrat_32);

  lv_style_init(&ui->font.large_48);
  lv_style_set_text_font(&ui->font.large_48, &lv_font_montserrat_48);

  lv_style_init(&ui->font.time);
  lv_style_set_text_font(&ui->font.time, &my_time_font);

  lv_style_init(&ui->font.icon);
  lv_style_set_bg_opa(&ui->font.icon, LV_OPA_TRANSP); // убираем фон
  lv_style_set_border_width(&ui->font.icon, 0); // убираем рамку
  lv_style_set_shadow_width(&ui->font.icon, 0); // убираем тень
  lv_style_set_text_font(&ui->font.icon, &my_symbols); // шрифт символов

  lv_style_init(&ui->font.nav_btn);
  lv_style_set_bg_opa(&ui->font.nav_btn, LV_OPA_TRANSP);
  lv_style_set_border_width(&ui->font.nav_btn, 0);
  lv_style_set_shadow_width(&ui->font.nav_btn, 0);
  lv_style_set_text_font(&ui->font.nav_btn, &my_symbols);
}

void apply_theme_dark(ui_main_menu_t *ui) {
  lv_style_reset(&ui->style.main);
  lv_style_set_bg_color(&ui->style.main,
                        lv_color_hex(0x0D1117)); // почти чёрный сверху
  lv_style_set_bg_grad_color(&ui->style.main,
                             lv_color_hex(0x1E3A5F)); // тёмно-синий снизу
  lv_style_set_bg_grad_dir(&ui->style.main, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&ui->style.main, LV_OPA_COVER);

  lv_style_reset(&ui->style.popup);
  lv_style_set_bg_color(&ui->style.popup,
                        lv_color_hex(0x161C24)); // тёмный, без градиента
  lv_style_set_bg_grad_dir(&ui->style.popup, LV_GRAD_DIR_NONE);
  lv_style_set_bg_opa(&ui->style.popup, LV_OPA_COVER);

  lv_style_reset(&ui->style.top_left);
  lv_style_set_bg_color(&ui->style.top_left, lv_color_hex(0x1A2E22));
  lv_style_set_border_color(&ui->style.top_left, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.top_left, 1);
  lv_style_set_radius(&ui->style.top_left, 12);

  lv_style_reset(&ui->style.bot_left);
  lv_style_set_bg_color(&ui->style.bot_left, lv_color_hex(0x2A2212));
  lv_style_set_border_color(&ui->style.bot_left, lv_color_hex(0xD48200));
  lv_style_set_border_width(&ui->style.bot_left, 1);
  lv_style_set_radius(&ui->style.bot_left, 12);

  lv_style_reset(&ui->style.top_right);
  lv_style_set_bg_color(&ui->style.top_right, lv_color_hex(0x1A1A2A));
  lv_style_set_border_color(&ui->style.top_right, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.top_right, 1);
  lv_style_set_radius(&ui->style.top_right, 12);

  lv_style_reset(&ui->style.bot_right);
  lv_style_set_bg_color(&ui->style.bot_right, lv_color_hex(0x161B22));
  lv_style_set_border_color(&ui->style.bot_right, lv_color_hex(0x30363D));
  lv_style_set_border_width(&ui->style.bot_right, 1);
  lv_style_set_radius(&ui->style.bot_right, 12);

  lv_style_reset(&ui->style.chart_co2);
  lv_style_set_bg_opa(&ui->style.chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&ui->style.chart_co2, lv_color_hex(0x1A2233));
  lv_style_set_bg_grad_dir(&ui->style.chart_co2, LV_GRAD_DIR_NONE);
  lv_style_set_border_color(&ui->style.chart_co2, lv_color_hex(0x2E4A6A));
  lv_style_set_border_width(&ui->style.chart_co2, 1);
  lv_style_set_radius(&ui->style.chart_co2, 8);

  lv_style_reset(&ui->style.meter_co2);
  lv_style_set_bg_color(&ui->style.meter_co2, lv_color_hex(0x1A2E22));
  lv_style_set_border_color(&ui->style.meter_co2, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.meter_co2, 1);

  lv_style_reset(&ui->style.category_btn);
  lv_style_set_bg_color(&ui->style.category_btn, lv_color_hex(0x1E2D3D));
  lv_style_set_bg_opa(&ui->style.category_btn, LV_OPA_COVER);

  // Шрифты
  lv_style_set_text_color(&ui->font.medium_32, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.small_24, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.large_48, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.time, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.very_small_20, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.icon, lv_color_hex(0x60A5FA));
  lv_style_set_text_color(&ui->font.nav_btn, lv_color_hex(0x60A5FA));
}

void apply_theme_light(ui_main_menu_t *ui) {
  lv_style_reset(&ui->style.main);
  lv_style_set_bg_color(&ui->style.main,
                        lv_color_hex(0xF0F4F8)); // светло-серый сверху
  lv_style_set_bg_grad_color(&ui->style.main,
                             lv_color_hex(0xB8D0E8)); // голубой снизу
  lv_style_set_bg_grad_dir(&ui->style.main, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&ui->style.main, LV_OPA_COVER);

  lv_style_reset(&ui->style.popup);
  lv_style_set_bg_color(&ui->style.popup,
                        lv_color_hex(0xE8EEF4)); // светлый, без градиента
  lv_style_set_bg_grad_dir(&ui->style.popup, LV_GRAD_DIR_NONE);
  lv_style_set_bg_opa(&ui->style.popup, LV_OPA_COVER);

  lv_style_reset(&ui->style.top_left);
  lv_style_set_bg_color(&ui->style.top_left, lv_color_hex(0xDCF5E7));
  lv_style_set_border_color(&ui->style.top_left, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.top_left, 1);
  lv_style_set_radius(&ui->style.top_left, 12);

  lv_style_reset(&ui->style.bot_left);
  lv_style_set_bg_color(&ui->style.bot_left, lv_color_hex(0xFFF3D6));
  lv_style_set_border_color(&ui->style.bot_left, lv_color_hex(0xD48200));
  lv_style_set_border_width(&ui->style.bot_left, 1);
  lv_style_set_radius(&ui->style.bot_left, 12);

  lv_style_reset(&ui->style.top_right);
  lv_style_set_bg_color(&ui->style.top_right, lv_color_hex(0xEDE9FF));
  lv_style_set_border_color(&ui->style.top_right, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.top_right, 1);
  lv_style_set_radius(&ui->style.top_right, 12);

  lv_style_reset(&ui->style.bot_right);
  lv_style_set_bg_color(&ui->style.bot_right, lv_color_hex(0x6B9FD4)); // небо
  lv_style_set_border_color(&ui->style.bot_right, lv_color_hex(0x4A80B8));
  lv_style_set_border_width(&ui->style.bot_right, 1);
  lv_style_set_radius(&ui->style.bot_right, 12);

  lv_style_reset(&ui->style.chart_co2);
  lv_style_set_bg_opa(&ui->style.chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&ui->style.chart_co2, lv_color_hex(0xEEF2F7));
  lv_style_set_bg_grad_dir(&ui->style.chart_co2, LV_GRAD_DIR_NONE);
  lv_style_set_border_color(&ui->style.chart_co2, lv_color_hex(0xB0BEC5));
  lv_style_set_border_width(&ui->style.chart_co2, 1);
  lv_style_set_radius(&ui->style.chart_co2, 8);

  lv_style_reset(&ui->style.meter_co2);
  lv_style_set_bg_color(&ui->style.meter_co2, lv_color_hex(0xEDE9FF));
  lv_style_set_border_color(&ui->style.meter_co2, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.meter_co2, 1);

  lv_style_reset(&ui->style.category_btn);
  lv_style_set_bg_color(&ui->style.category_btn, lv_color_hex(0xD0DCE8));
  lv_style_set_bg_opa(&ui->style.category_btn, LV_OPA_COVER);

  // Шрифты — только text_color, font не трогаем
  lv_style_set_text_color(&ui->font.very_small_20, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.medium_32, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.small_24, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.large_48, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.time, lv_color_hex(0x1A1A2A));

  lv_style_set_text_color(&ui->font.icon, lv_color_hex(0x4A80B8));
  lv_style_set_text_color(&ui->font.nav_btn, lv_color_hex(0x4A80B8));
}

void ui_apply_theme(ui_main_menu_t *ui) {
  switch (ui->settings.switch_.theme_mode) {
  case 0: // Auto
    ui->settings.switch_.theme_last_is_day = get_is_day();
    ui->settings.switch_.theme_last_is_day ? apply_theme_light(ui)
                                           : apply_theme_dark(ui);
    break;
  case 1:
    apply_theme_light(ui);
    break; // Hell
  case 2:
    apply_theme_dark(ui);
    break; // Dunkel
    
  }
  lv_obj_report_style_change(&ui->style.main);
  lv_obj_report_style_change(&ui->style.popup);
  lv_obj_report_style_change(&ui->style.top_left);
  lv_obj_report_style_change(&ui->style.bot_left);
  lv_obj_report_style_change(&ui->style.top_right);
  lv_obj_report_style_change(&ui->style.bot_right);
  lv_obj_report_style_change(&ui->font.very_small_20);
  lv_obj_report_style_change(&ui->font.small_24);
  lv_obj_report_style_change(&ui->font.medium_32);
  lv_obj_report_style_change(&ui->font.large_48);
  lv_obj_report_style_change(&ui->font.nav_btn);
  lv_obj_report_style_change(&ui->font.time);
  lv_obj_report_style_change(&ui->font.icon);
}

void init_styles(ui_main_menu_t *ui) {
  lv_style_init(&ui->style.main);
  lv_style_init(&ui->style.popup);
  lv_style_init(&ui->style.top_left);
  lv_style_init(&ui->style.bot_left);
  lv_style_init(&ui->style.top_right);
  lv_style_init(&ui->style.bot_right);
  lv_style_init(&ui->style.meter_co2);
  lv_style_init(&ui->style.chart_co2);
  lv_style_init(&ui->style.category_btn);

  ui_apply_theme(ui); // применяем нужную тему
}