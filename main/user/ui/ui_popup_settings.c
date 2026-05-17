
#include "ui_popup_settings.h"
#include "backlight.h"
#include "nvs_user.h"
#include "ui_blocks.h"
#include "ui_core.h"
#include "ui_theme.h"
#include "ui_user_config.h"
#include "ui_widgets.h"
#include "ui_wifi.h"
#include "wifi_user.h"

extern volatile bool ota_in_progress;

ui_main_menu_t *g_ui = &ui;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI Settings Popup
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_create_settings_popup(ui_main_menu_t *ui) {
  // --- fullscreen попап ---
  ui->settings.popup = create_background(
      ui->screen, LVGL_PORT_H_RES, LVGL_PORT_V_RES, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_add_style(ui->settings.popup, &ui->style.popup, 0);
  lv_obj_set_scrollbar_mode(ui->settings.popup, LV_SCROLLBAR_MODE_OFF);
  lv_obj_clear_flag(ui->settings.popup, LV_OBJ_FLAG_SCROLLABLE);

  // --- кнопка Close (всегда видна) ---
  ui->settings.btn_close =
      create_btn_cb(ui->settings.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -10, 10,
                    btn_settings_close_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->settings.btn_close, LV_SYMBOL_CLOSE, 0);

  // --- кнопка Back (скрыта на home) ---
  ui->settings.btn_back =
      create_btn_cb(ui->settings.popup, 50, 50, LV_ALIGN_TOP_LEFT, 10, 10,
                    btn_settings_back_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->settings.btn_back, LV_SYMBOL_LEFT, 0);
  lv_obj_add_flag(ui->settings.btn_back, LV_OBJ_FLAG_HIDDEN);

  // --- заголовок ---
  create_text("Einstellungen", ui->settings.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, 15, ui);

  // --- page_home: список категорий ---
  ui->settings.page_home = lv_obj_create(ui->settings.popup);
  lv_obj_remove_style_all(ui->settings.page_home);
  lv_obj_set_size(ui->settings.page_home, LVGL_PORT_H_RES,
                  LVGL_PORT_V_RES - 80);
  lv_obj_align(ui->settings.page_home, LV_ALIGN_BOTTOM_MID, 0, 30);
  lv_obj_set_scroll_dir(ui->settings.page_home, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ui->settings.page_home, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_flex_flow(ui->settings.page_home, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(ui->settings.page_home, 10, 0);
  lv_obj_set_style_pad_all(ui->settings.page_home, 15, 0);

  // --- page_content: контент категории (скрыт) ---
  ui->settings.page_content = lv_obj_create(ui->settings.popup);
  lv_obj_remove_style_all(ui->settings.page_content);
  lv_obj_set_size(ui->settings.page_content, LVGL_PORT_H_RES,
                  LVGL_PORT_V_RES - 80);
  lv_obj_align(ui->settings.page_content, LV_ALIGN_BOTTOM_MID, 0, 30);
  lv_obj_set_scroll_dir(ui->settings.page_content, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(ui->settings.page_content, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_pad_all(ui->settings.page_content, 15, 0);
  lv_obj_add_flag(ui->settings.page_content, LV_OBJ_FLAG_HIDDEN);

  // --- категории ---
  static const char *cat_labels[] = {
      LV_SYMBOL_IMAGE " Display",
      LV_SYMBOL_WIFI " WLAN",
      "\xEF\x82\x93"
      " Bluetooth",
      LV_SYMBOL_GPS " Wetter",
      LV_SYMBOL_HOME " Sensoren",
      LV_SYMBOL_DOWNLOAD " Update",
      LV_SYMBOL_LIST " Info",
  };
  static const settings_page_t cat_pages[] = {
      SETTINGS_PAGE_DISPLAY, SETTINGS_PAGE_WIFI,    SETTINGS_PAGE_BLUETOOTH,
      SETTINGS_PAGE_WEATHER, SETTINGS_PAGE_SENSORS, SETTINGS_PAGE_UPDATE,
      SETTINGS_PAGE_INFO,
  };

  for (int i = 0; i < SETTINGS_PAGE_LAST_ELEMENT - 1; i++) {
    lv_obj_t *btn = lv_btn_create(ui->settings.page_home);
    lv_obj_set_size(btn, LVGL_PORT_H_RES - 50, 60);
    lv_obj_add_style(btn, &ui->style.popup, 0);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0x2D5A8E), 0);
    lv_obj_set_style_shadow_width(btn, 0, 0);
    lv_obj_add_style(btn, &ui->style.category_btn, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2D6AB4), LV_STATE_PRESSED);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, cat_labels[i]);
    lv_obj_add_style(lbl, &ui->font.medium_32, 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 10, 0);

    lv_obj_t *arrow = lv_label_create(btn);
    lv_label_set_text(arrow, LV_SYMBOL_RIGHT);
    lv_obj_add_style(arrow, &ui->font.medium_32, 0);
    lv_obj_align(arrow, LV_ALIGN_RIGHT_MID, -10, 0);

    lv_obj_add_event_cb(btn, btn_settings_category_event_handler,
                        LV_EVENT_CLICKED, ui);
    lv_obj_set_user_data(btn, (void *)(uintptr_t)cat_pages[i]);
  }

  ui->settings.current_page = SETTINGS_PAGE_HOME;
}

void ui_create_settings_popup_btns(ui_main_menu_t *ui) {
  ui->settings.btn_open = create_btn_icon(
      ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
      BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_2,
      BLOCK_BOT_MID_Y_START_SYMBOLS, btn_settings_open_popup_event_handler, ui,
      LV_SYMBOL_SETTINGS, NULL, &lv_font_montserrat_32, "Einstellungen", ui);
}

void btn_settings_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (ui->settings.popup != NULL && lv_obj_is_valid(ui->settings.popup))
    return;

  hide_all_blocks(ui);
  ui_create_settings_popup(ui);
}

void btn_settings_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->settings.popup);

  ui->settings.popup = NULL;
  ui->settings.btn_close = NULL;
  ui->settings.standby_btnmatrix = NULL;
  ui->settings.standby_desc_label = NULL;
  ui->settings.theme_btnmatrix = NULL;
  ui->settings.theme_desc_label = NULL;
  ui->settings.co2_btnmatrix = NULL;
  ui->settings.co2_desc_label = NULL;
  ui->settings.backlight_btnmatrix = NULL;
  ui->settings.backlight_desc_label = NULL;
  ui->settings.backlight_slider = NULL;
  ui->settings.backlight_pct_label = NULL;
  ui->settings.page_home = NULL;
  ui->settings.page_content = NULL;
  ui->settings.btn_back = NULL;
  ui->settings.current_page = SETTINGS_PAGE_HOME;
  ui->settings.ota_btn = NULL;
  ui->settings.ota_status_label = NULL;
  ui->wifi.keyboard = NULL;
  ui->wifi.connected_ssid_label = NULL;
  ui->wifi.scan_list = NULL;
  ui->wifi.scan_status_label = NULL;
  ui->wifi.pass_popup = NULL;
  ui->wifi.pass_popup_ssid_label = NULL;
  ui->wifi.ta_pass = NULL;
  ui->wifi.keyboard = NULL;
  ui->wifi.scan_count = 0;
  ui->wifi.pending_ssid[0] = '\0';
  ui->wifi.ta_pass = NULL;
  ui->weather.settings_popup.city_label = NULL;
  ui->weather.settings_popup.btn_open_city_list = NULL;
  ui->weather.settings_popup.citys_list = NULL;

  show_all_blocks(ui);
}

void btn_settings_back_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_clean(ui->settings.page_content);

  ui->settings.standby_btnmatrix = NULL;
  ui->settings.standby_desc_label = NULL;
  ui->settings.theme_btnmatrix = NULL;
  ui->settings.theme_desc_label = NULL;
  ui->settings.co2_btnmatrix = NULL;
  ui->settings.co2_desc_label = NULL;
  ui->settings.backlight_btnmatrix = NULL;
  ui->settings.backlight_desc_label = NULL;
  ui->settings.backlight_slider = NULL;
  ui->settings.backlight_pct_label = NULL;
  ui->settings.ota_btn = NULL;
  ui->settings.ota_status_label = NULL;
  ui->wifi.connected_ssid_label = NULL;
  ui->wifi.scan_list = NULL;
  ui->wifi.scan_status_label = NULL;
  ui->wifi.pass_popup = NULL;
  ui->wifi.pass_popup_ssid_label = NULL;
  ui->wifi.ta_pass = NULL;
  ui->wifi.keyboard = NULL;
  ui->wifi.scan_count = 0;
  ui->wifi.pending_ssid[0] = '\0';
  ui->wifi.keyboard = NULL;
  ui->wifi.ta_pass = NULL;
  ui->weather.settings_popup.city_label = NULL;
  ui->weather.settings_popup.btn_open_city_list = NULL;
  ui->weather.settings_popup.citys_list = NULL;

  lv_obj_clear_flag(ui->settings.page_home, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->settings.page_content, LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(ui->settings.btn_back, LV_OBJ_FLAG_HIDDEN);

  ui->settings.current_page = SETTINGS_PAGE_HOME;
}

void btn_settings_category_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_t *btn = lv_event_get_target(e);
  settings_page_t page = (settings_page_t)(uintptr_t)lv_obj_get_user_data(btn);

  ui->settings.current_page = page;

  lv_obj_add_flag(ui->settings.page_home, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui->settings.page_content, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(ui->settings.btn_back, LV_OBJ_FLAG_HIDDEN);

  lv_obj_clean(ui->settings.page_content);
  lv_obj_t *lbl = lv_label_create(ui->settings.page_content);
  lv_obj_add_style(lbl, &ui->font.medium_32, 0);
  lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

  switch (page) {
  case SETTINGS_PAGE_DISPLAY:
    lv_obj_del(lbl);
    ui_create_settigs_display(ui);
    break;
  case SETTINGS_PAGE_WIFI:
    lv_obj_del(lbl);
    ui_create_settigs_wifi(ui);
    break;
  case SETTINGS_PAGE_BLUETOOTH:
    lv_label_set_text(lbl, LV_SYMBOL_BLUETOOTH " Bluetooth");
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 20);
    create_text("In Entwicklung...", ui->settings.page_content,
                STYLE_TEXT_SMALL, LV_ALIGN_CENTER, 0, 0, ui);
    break;
  case SETTINGS_PAGE_WEATHER:
    lv_obj_del(lbl);
    ui_create_settigs_weather(ui);
    break;
  case SETTINGS_PAGE_SENSORS:
    lv_obj_del(lbl);
    ui_create_settigs_sensors(ui);
    break;
  case SETTINGS_PAGE_UPDATE:
    lv_obj_del(lbl);
    create_text("Firmware Update", ui->settings.page_content, STYLE_TEXT_SMALL,
                LV_ALIGN_TOP_MID, 0, 10, ui);
    ui->settings.ota_btn =
        create_btn_cb(ui->settings.page_content, 200, 60, LV_ALIGN_TOP_LEFT, 10,
                      80, ota_btn_event_cb, NULL);
    lv_obj_t *ota_lbl = lv_label_create(ui->settings.ota_btn);
    lv_label_set_text(ota_lbl, LV_SYMBOL_DOWNLOAD " Update");
    lv_obj_center(ota_lbl);
    ui->settings.ota_status_label = create_label(
        ui->settings.page_content, "Bereit", LV_ALIGN_TOP_LEFT, 225, 95);
    lv_obj_add_style(ui->settings.ota_status_label, &ui->font.very_small_20, 0);
    break;
  case SETTINGS_PAGE_INFO:
    lv_label_set_text(lbl, LV_SYMBOL_LIST " Info");
    lv_obj_align(lbl, LV_ALIGN_TOP_MID, 0, 20);
    create_text("Geraet: ESP32-S3\n"
                "Display: 7\"\n"
                "Framework: ESP-IDF / LVGL 8.4\n"
                "Wetter: Open-Meteo\n"
                "Software Version: " CURRENT_SOFT_VERSION,
                ui->settings.page_content, STYLE_TEXT_SMALL, LV_ALIGN_CENTER, 0,
                0, ui);
    break;
  default:
    break;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Display Settings
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_create_settigs_display(ui_main_menu_t *ui) {
  const lv_coord_t W = LVGL_PORT_H_RES - 60;
  const lv_coord_t X = 20;

  create_text("Display", ui->settings.page_content, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, 10, ui);

  // --- Helligkeit ---
  create_text("Helligkeit:", ui->settings.page_content, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, X, 60, ui);

  static const char *bl_map[] = {"Manuell", "Auto", "Zeitplan", ""};
  ui->settings.backlight_btnmatrix =
      lv_btnmatrix_create(ui->settings.page_content);
  lv_obj_set_size(ui->settings.backlight_btnmatrix, W, 90);
  lv_obj_align(ui->settings.backlight_btnmatrix, LV_ALIGN_TOP_LEFT, X, 100);
  lv_btnmatrix_set_map(ui->settings.backlight_btnmatrix, bl_map);
  lv_obj_set_style_text_font(ui->settings.backlight_btnmatrix,
                             &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.backlight_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.backlight_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.backlight_btnmatrix,
                      backlight_btnmatrix_event_cb, LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.backlight_desc_label =
      create_label(ui->settings.page_content, "", LV_ALIGN_TOP_LEFT, X, 200);
  lv_obj_add_style(ui->settings.backlight_desc_label, &ui->font.very_small_20,
                   0);

  ui->settings.backlight_slider = lv_slider_create(ui->settings.page_content);
  lv_obj_set_size(ui->settings.backlight_slider, W - 120, 40);
  lv_obj_align(ui->settings.backlight_slider, LV_ALIGN_TOP_LEFT, 40, 240);
  lv_slider_set_range(ui->settings.backlight_slider, 5, 100);
  lv_slider_set_value(ui->settings.backlight_slider,
                      ui->settings.switch_.backlight_pct, LV_ANIM_OFF);
  lv_obj_add_event_cb(ui->settings.backlight_slider, backlight_slider_event_cb,
                      LV_EVENT_VALUE_CHANGED, ui);

  lv_obj_set_style_bg_color(ui->settings.backlight_slider,
                            lv_color_hex(0x555555),
                            LV_PART_INDICATOR | LV_STATE_DISABLED);
  lv_obj_set_style_bg_color(ui->settings.backlight_slider,
                            lv_color_hex(0x555555),
                            LV_PART_KNOB | LV_STATE_DISABLED);

  ui->settings.backlight_pct_label = create_label(
      ui->settings.page_content, "", LV_ALIGN_TOP_LEFT, W - 50, 248);
  lv_obj_add_style(ui->settings.backlight_pct_label, &ui->font.small_24, 0);

  // --- Standby ---
  create_text("Standby:", ui->settings.page_content, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, X, 300, ui);

  static const char *standby_map[] = {"Ein", "Auto", "Auto Nachts", ""};
  ui->settings.standby_btnmatrix =
      lv_btnmatrix_create(ui->settings.page_content);
  lv_obj_set_size(ui->settings.standby_btnmatrix, W, 90);
  lv_obj_align(ui->settings.standby_btnmatrix, LV_ALIGN_TOP_LEFT, X, 340);
  lv_btnmatrix_set_map(ui->settings.standby_btnmatrix, standby_map);
  lv_obj_set_style_text_font(ui->settings.standby_btnmatrix,
                             &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.standby_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.standby_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.standby_btnmatrix,
                      standby_btnmatrix_event_cb, LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.standby_desc_label =
      create_label(ui->settings.page_content, "", LV_ALIGN_TOP_LEFT, X, 440);
  lv_obj_add_style(ui->settings.standby_desc_label, &ui->font.very_small_20, 0);

  // --- Thema ---
  create_text("Thema:", ui->settings.page_content, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, X, 480, ui);

  static const char *theme_map[] = {"Auto", "Hell", "Dunkel", ""};
  ui->settings.theme_btnmatrix = lv_btnmatrix_create(ui->settings.page_content);
  lv_obj_set_size(ui->settings.theme_btnmatrix, W, 90);
  lv_obj_align(ui->settings.theme_btnmatrix, LV_ALIGN_TOP_LEFT, X, 520);
  lv_btnmatrix_set_map(ui->settings.theme_btnmatrix, theme_map);
  lv_obj_set_style_text_font(ui->settings.theme_btnmatrix,
                             &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.theme_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.theme_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.theme_btnmatrix, theme_btnmatrix_event_cb,
                      LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.theme_desc_label =
      create_label(ui->settings.page_content, "", LV_ALIGN_TOP_LEFT, X, 620);
  lv_obj_add_style(ui->settings.theme_desc_label, &ui->font.very_small_20, 0);

  // --- применяем сохранённые значения ---
  main_settings_load(
      &ui->settings.switch_.standby_mode, &ui->settings.switch_.backlight_pct,
      &ui->settings.switch_.backlight_mode, &ui->settings.switch_.theme_mode,
      &ui->settings.switch_.co2_mode);

  lv_btnmatrix_set_btn_ctrl(ui->settings.backlight_btnmatrix,
                            ui->settings.switch_.backlight_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);
  if (ui->settings.switch_.backlight_mode != 0)
    lv_obj_add_state(ui->settings.backlight_slider, LV_STATE_DISABLED);

  static const char *bl_descs[] = {
      "Feste Helligkeit per Schieberegler.", "Tagsueber 80%, nachts 5%.",
      "Sanfter Verlauf: Morgen, Tag, Abend, Nacht."};
  lv_label_set_text(ui->settings.backlight_desc_label,
                    bl_descs[ui->settings.switch_.backlight_mode]);

  char bl_buf[8];
  snprintf(bl_buf, sizeof(bl_buf), "%d%%", ui->settings.switch_.backlight_pct);
  lv_label_set_text(ui->settings.backlight_pct_label, bl_buf);

  lv_btnmatrix_set_btn_ctrl(ui->settings.standby_btnmatrix,
                            ui->settings.switch_.standby_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);

  static const char *standby_descs[] = {
      "Immer eingeschaltet.", "Aus nach 180 Sek.", "Aus Nachts nach 180 Sek."};
  lv_label_set_text(ui->settings.standby_desc_label,
                    standby_descs[ui->settings.switch_.standby_mode]);

  lv_btnmatrix_set_btn_ctrl(ui->settings.theme_btnmatrix,
                            ui->settings.switch_.theme_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);

  static const char *theme_descs[] = {
      "Automatisch: Tagsueber hell, nachts dunkel.", "Immer helles Design.",
      "Immer dunkles Design."};
  lv_label_set_text(ui->settings.theme_desc_label,
                    theme_descs[ui->settings.switch_.theme_mode]);
}

void backlight_btnmatrix_event_cb(lv_event_t *e) {
  char buf[16];
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.backlight_mode = (uint8_t)btn_id;

  if (btn_id == 0) {
    lv_obj_clear_state(ui->settings.backlight_slider, LV_STATE_DISABLED);
    backlight_set(ui->settings.switch_.backlight_pct);
    snprintf(buf, sizeof(buf), "%d%%", ui->settings.switch_.backlight_pct);
    lv_label_set_text(ui->settings.backlight_pct_label, buf);
  } else {
    lv_obj_add_state(ui->settings.backlight_slider, LV_STATE_DISABLED);
    uint8_t pct =
        (btn_id == 1) ? backlight_get_auto_pct() : backlight_get_zeitplan_pct();
    backlight_set(pct);
    snprintf(buf, sizeof(buf), "%d%%", pct);
    lv_label_set_text(ui->settings.backlight_pct_label, buf);
  }
  static const char *bl_descs[] = {
      "Feste Helligkeit per Schieberegler.", "Tagsueber 80%, nachts 5%.",
      "Sanfter Verlauf: Morgen, Tag, Abend, Nacht."};
  lv_label_set_text(ui->settings.backlight_desc_label, bl_descs[btn_id]);

  main_settings_save(
      ui->settings.switch_.standby_mode, ui->settings.switch_.backlight_pct,
      ui->settings.switch_.backlight_mode, ui->settings.switch_.theme_mode,
      ui->settings.switch_.co2_mode);
}

void backlight_slider_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *slider = lv_event_get_target(e);
  uint8_t pct = (uint8_t)lv_slider_get_value(slider);
  ui->settings.switch_.backlight_pct = pct;

  char buf[16];
  snprintf(buf, sizeof(buf), "%d%%", pct);
  lv_label_set_text(ui->settings.backlight_pct_label, buf);

  backlight_set(pct);
  main_settings_save(
      ui->settings.switch_.standby_mode, ui->settings.switch_.backlight_pct,
      ui->settings.switch_.backlight_mode, ui->settings.switch_.theme_mode,
      ui->settings.switch_.co2_mode);
}

void standby_btnmatrix_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.standby_mode = (uint8_t)btn_id;

  static const char *descs[] = {"Immer eingeschaltet.", "Aus nach 180 Sek.",
                                "Aus Nachts nach 180 Sek."};
  lv_label_set_text(ui->settings.standby_desc_label, descs[btn_id]);

  main_settings_save(
      ui->settings.switch_.standby_mode, ui->settings.switch_.backlight_pct,
      ui->settings.switch_.backlight_mode, ui->settings.switch_.theme_mode,
      ui->settings.switch_.co2_mode);
}

void theme_btnmatrix_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.theme_mode = (uint8_t)btn_id;

  static const char *descs[] = {"Automatisch: Tagsueber hell, nachts dunkel.",
                                "Immer helles Design.",
                                "Immer dunkles Design."};
  lv_label_set_text(ui->settings.theme_desc_label, descs[btn_id]);

  switch (btn_id) {
  case 0:
    ui->settings.switch_.theme_last_is_day = get_is_day();
    ui->settings.switch_.theme_last_is_day ? apply_theme_light(ui)
                                           : apply_theme_dark(ui);
    break;
  case 1:
    apply_theme_light(ui);
    break;
  case 2:
    apply_theme_dark(ui);
    break;
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

  main_settings_save(
      ui->settings.switch_.standby_mode, ui->settings.switch_.backlight_pct,
      ui->settings.switch_.backlight_mode, ui->settings.switch_.theme_mode,
      ui->settings.switch_.co2_mode);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WiFi Settings
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_create_settigs_wifi(ui_main_menu_t *ui) {

  if (get_wifi_status() != WIFI_CONNECTED) {
    wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(300));
  }
  esp_err_t err = wifi_scan_start();
  ESP_LOGI(TAG, "wifi_scan_start ret=%d", err);

  lv_obj_t *p = ui->settings.page_content;

  // --- текущее подключение ---
  lv_obj_t *conn_block = lv_obj_create(p);
  lv_obj_set_size(conn_block, lv_obj_get_width(p) - 20, 60);
  lv_obj_align(conn_block, LV_ALIGN_TOP_MID, 0, 5);
  lv_obj_add_style(conn_block, &ui->style.popup, 0);
  lv_obj_set_style_radius(conn_block, 10, 0);
  lv_obj_set_style_border_width(conn_block, 1, 0);
  lv_obj_set_style_border_color(conn_block, lv_color_hex(0x2D5A8E), 0);
  lv_obj_set_style_pad_all(conn_block, 8, 0);
  lv_obj_clear_flag(conn_block, LV_OBJ_FLAG_SCROLLABLE);

  ui->wifi.connected_ssid_label = lv_label_create(conn_block);
  lv_obj_add_style(ui->wifi.connected_ssid_label, &ui->font.medium_32, 0);
  const char *cur_ssid = get_wifi_ssid();
  lv_label_set_text(ui->wifi.connected_ssid_label,
                    cur_ssid ? cur_ssid : "Nicht verbunden");
  lv_obj_align(ui->wifi.connected_ssid_label, LV_ALIGN_LEFT_MID, 5, 0);

  int16_t rssi = get_wifi_rssi();
  uint8_t bars = (cur_ssid != NULL) ? rssi_to_bars(rssi) : 0;
  for (int i = 0; i < 4; i++) {
    lv_obj_t *bar = lv_obj_create(conn_block);
    lv_coord_t bar_h = 8 + i * 7;
    lv_obj_set_size(bar, 10, bar_h);
    lv_obj_align(bar, LV_ALIGN_RIGHT_MID, -5 - (3 - i) * 16, (29 - bar_h) / 2);
    lv_obj_set_style_radius(bar, 2, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_bg_color(
        bar, (i < bars) ? lv_color_hex(0x4CAF50) : lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    ui->wifi.signal_bars[i] = bar;
  }

  ui->wifi.scan_status_label = lv_label_create(p);
  lv_obj_add_style(ui->wifi.scan_status_label, &ui->font.very_small_20, 0);
  lv_label_set_text(ui->wifi.scan_status_label, "Suche...");
  lv_obj_align(ui->wifi.scan_status_label, LV_ALIGN_TOP_MID, 0, 75);

  ui->wifi.scan_list = lv_list_create(p);
  lv_obj_set_size(ui->wifi.scan_list, lv_obj_get_width(p) - 20,
                  lv_obj_get_height(p) - 145);
  lv_obj_align(ui->wifi.scan_list, LV_ALIGN_TOP_MID, 0, 100);
  lv_obj_add_style(ui->wifi.scan_list, &ui->style.popup, 0);
  lv_obj_set_style_radius(ui->wifi.scan_list, 10, 0);
  lv_obj_set_style_border_width(ui->wifi.scan_list, 1, 0);
  lv_obj_set_style_border_color(ui->wifi.scan_list, lv_color_hex(0x2D5A8E), 0);

  wifi_scan_start();
  lv_timer_t *t = lv_timer_create(wifi_scan_timer_cb, 500, ui);
  lv_timer_set_repeat_count(t, 40);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Weather Settings
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_create_settigs_weather(ui_main_menu_t *ui) {
  lv_obj_t *p = ui->settings.page_content;

  create_text("Stadt:", p, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT, 15, 10, ui);

  ui->weather.settings_popup.city_label =
      create_label(p,
                   ui->weather.settings_popup
                       .cities_de[ui->weather.settings_popup.saved_city]
                       .name,
                   LV_ALIGN_TOP_LEFT, 220, 10);
  lv_obj_add_style(ui->weather.settings_popup.city_label, &ui->font.medium_32,
                   0);

  ui->weather.settings_popup.btn_open_city_list =
      create_btn_cb(p, 50, 50, LV_ALIGN_TOP_LEFT, 500, 10,
                    btn_weather_open_list_city_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.settings_popup.btn_open_city_list,
                              LV_SYMBOL_GPS, 0);

  ui_create_city_list_weather(ui);

  lv_obj_t *info_label =
      create_label(p,
                   "Datenquelle: Open-Meteo (open-meteo.com)\n"
                   "Aktuelle Werte sind berechnet, keine Echtzeitmessung.\n"
                   "Vorhersagen koennen von der Realitaet abweichen.",
                   LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_font(info_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(info_label, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(info_label, LVGL_PORT_H_RES - 40);
}

void ui_create_city_list_weather(ui_main_menu_t *ui) {
  ui->weather.settings_popup.citys_list =
      lv_list_create(ui->settings.page_content);
  lv_obj_set_size(ui->weather.settings_popup.citys_list, 350, 250);
  lv_obj_center(ui->weather.settings_popup.citys_list);

  for (int i = 0; i < ui->weather.settings_popup.city_count; i++) {
    lv_obj_t *btn =
        lv_list_add_btn(ui->weather.settings_popup.citys_list, NULL,
                        ui->weather.settings_popup.cities_de[i].name);
    lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
    lv_obj_add_event_cb(btn, set_current_city_weather_event_handler,
                        LV_EVENT_CLICKED, ui);
  }

  if (lv_obj_is_valid(ui->weather.settings_popup.city_label))
    lv_label_set_text(ui->weather.settings_popup.city_label,
                      ui->weather.settings_popup
                          .cities_de[ui->weather.settings_popup.saved_city]
                          .name);

  set_visible(ui->weather.settings_popup.citys_list, false);
}

void btn_weather_open_list_city_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (lv_obj_is_valid(ui->weather.settings_popup.citys_list))
    set_visible(ui->weather.settings_popup.citys_list, true);
}

void set_current_city_weather_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_CLICKED)
    return;

  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_t *btn = lv_event_get_target(e);
  uint16_t city = (uint16_t)(uintptr_t)lv_obj_get_user_data(btn);

  weather_settings_save(city);
  ui->weather.settings_popup.saved_city = city;

  if (lv_obj_is_valid(ui->weather.settings_popup.citys_list))
    set_visible(ui->weather.settings_popup.citys_list, false);

  if (lv_obj_is_valid(ui->weather.settings_popup.city_label))
    lv_label_set_text(ui->weather.settings_popup.city_label,
                      ui->weather.settings_popup.cities_de[city].name);

  build_weather_url(city);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Sensors Settings
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ui_create_settigs_sensors(ui_main_menu_t *ui) {
	 const lv_coord_t W = LVGL_PORT_H_RES - 60;
  const lv_coord_t X = 20;
	
  lv_obj_t *p = ui->settings.page_content;
  
  create_text("Luftqualitaetssensor:", ui->settings.page_content, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, X, 60, ui);


  create_text("Sensoren", p, STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID,
              0, 10, ui);

  static const char *co2_map[] = {"Sensitiv", "Normal", "Robust", ""};
  ui->settings.co2_btnmatrix = lv_btnmatrix_create(p);
  lv_obj_set_size(ui->settings.co2_btnmatrix, W, 90);
  lv_obj_align(ui->settings.co2_btnmatrix, LV_ALIGN_TOP_LEFT, X, 100);
  lv_btnmatrix_set_map(ui->settings.co2_btnmatrix, co2_map);
  lv_obj_set_style_text_font(ui->settings.co2_btnmatrix, &lv_font_montserrat_20,
                             LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.co2_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.co2_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.co2_btnmatrix, co2_btnmatrix_event_cb,
                      LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.co2_desc_label = create_label(p, "", LV_ALIGN_TOP_LEFT, X, 200);
  lv_obj_add_style(ui->settings.co2_desc_label, &ui->font.very_small_20, 0);

  lv_btnmatrix_set_btn_ctrl(ui->settings.co2_btnmatrix,
                            ui->settings.switch_.co2_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);

  static const char *co2_descs[] = {
      "400-2400 ppm: fuer Schlafraeume und Bueros.",
      "400-4000 ppm: fuer Wohnraeume.",
      "400-6000 ppm: fuer Industrie und Lager."};
  lv_label_set_text(ui->settings.co2_desc_label,
                    co2_descs[ui->settings.switch_.co2_mode]);

  lv_obj_t *info_label = create_label(
      p,

      "Der Luftqualitaetssensor erfasst fluechtige organische Verbindungen (VOCs)\n"
      "und berechnet daraus einen geschaetzten CO2-Aequivalentwert (eCO2)\n"
      "- keine direkte CO2-Messung.",
      LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_obj_set_style_text_font(info_label, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(info_label, lv_color_hex(0x888888), 0);
  lv_obj_set_style_text_align(info_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_label_set_long_mode(info_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(info_label, LVGL_PORT_H_RES - 40);
}

void co2_btnmatrix_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.co2_mode = (uint8_t)btn_id;

  static const char *descs[] = {"400-2400 ppm: fuer Schlafraeume und Bueros.",
                                "400-4000 ppm: fuer Wohnraeume.",
                                "400-6000 ppm: fuer Industrie und Lager."};
  lv_label_set_text(ui->settings.co2_desc_label, descs[btn_id]);

  main_settings_save(
      ui->settings.switch_.standby_mode, ui->settings.switch_.backlight_pct,
      ui->settings.switch_.backlight_mode, ui->settings.switch_.theme_mode,
      ui->settings.switch_.co2_mode);
  update_co2_chart_labels(ui);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OTA Settings
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ota_progress_cb(ota_state_t state, int progress_pct) {
  if (!g_ui || !g_ui->settings.ota_status_label)
    return;
  if (!lv_obj_is_valid(g_ui->settings.ota_status_label))
    return;

  char buf[48];
  switch (state) {
  case OTA_STATE_CHECKING:
    snprintf(buf, sizeof(buf), "Verbinde...");
    break;
  case OTA_STATE_DOWNLOADING:
    snprintf(buf, sizeof(buf), "Lade... %d%%", progress_pct);
    break;
  case OTA_STATE_SUCCESS:
    snprintf(buf, sizeof(buf), "Fertig! Neustart...");
    break;
  case OTA_STATE_FAILED:
    snprintf(buf, sizeof(buf), "Fehler!");
    break;
  default:
    snprintf(buf, sizeof(buf), "Bereit");
    break;
  }
  lv_label_set_text(g_ui->settings.ota_status_label, buf);
}

void ota_start_timer_cb(lv_timer_t *t) {
  lv_timer_del(t);
  lv_obj_t *overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(overlay, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
  lv_obj_set_style_border_width(overlay, 0, 0);
  lv_obj_set_style_radius(overlay, 0, 0);
  lv_obj_set_style_pad_all(overlay, 0, 0);
  ota_start(OTA_FIRMWARE_URL, ota_progress_cb);
}

void ota_btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;

  lv_obj_t *overlay = lv_obj_create(lv_scr_act());
  lv_obj_set_size(overlay, LV_HOR_RES, LV_VER_RES);
  lv_obj_align(overlay, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(overlay, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(overlay, LV_OPA_90, 0);
  lv_obj_set_style_border_width(overlay, 0, 0);

  lv_obj_t *lbl = lv_label_create(overlay);
  lv_label_set_text(lbl, "Firmware-Update wird gestartet.\n"
                         "Das Gerat wird neu gestartet.\n\n"
                         "Bitte warten...");
  lv_obj_center(lbl);
  lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);

  lv_timer_create(ota_start_timer_cb, 3000, NULL);
}
