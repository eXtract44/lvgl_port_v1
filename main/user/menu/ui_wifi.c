
#include "ui_wifi.h"
#include "ui_core.h"
/////////////////////////////////////////////////wifi events
// ---------------------------------------------------------------------------
// WiFi scan page — helpers
// ---------------------------------------------------------------------------

uint8_t rssi_to_bars(int16_t rssi) {
  if (rssi >= -55)
    return 4;
  if (rssi >= -67)
    return 3;
  if (rssi >= -78)
    return 2;
  return 1;
}

void wifi_update_signal_bars(ui_main_menu_t *ui, uint8_t bars) {
  for (int i = 0; i < 4; i++) {
    if (!lv_obj_is_valid(ui->wifi.signal_bars[i]))
      continue;
    bool active = (i < bars);
    lv_obj_set_style_bg_color(
        ui->wifi.signal_bars[i],
        active ? lv_color_hex(0x4CAF50) : lv_color_hex(0x444444), 0);
  }
}

// ---------------------------------------------------------------------------
// Password popup
// ---------------------------------------------------------------------------

void wifi_pass_popup_connect_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_READY) {
    const char *pass = lv_textarea_get_text(ui->wifi.ta_pass);
    wifi_connect(ui->wifi.pending_ssid, pass);

    if (lv_obj_is_valid(ui->wifi.connected_ssid_label))
      lv_label_set_text(ui->wifi.connected_ssid_label, "verbinde...");

    lv_timer_t *t = lv_timer_create(wifi_check_timer_cb, 500, ui);
    lv_timer_set_repeat_count(t, 10);

    if (lv_obj_is_valid(ui->wifi.keyboard))
      lv_obj_del(ui->wifi.keyboard);
    if (lv_obj_is_valid(ui->wifi.ta_pass))
      lv_obj_del(ui->wifi.ta_pass);
    if (lv_obj_is_valid(ui->wifi.pass_popup))
      lv_obj_del(ui->wifi.pass_popup);

    ui->wifi.keyboard = NULL;
    ui->wifi.ta_pass = NULL;
    ui->wifi.pass_popup = NULL;
    ui->wifi.pass_popup_ssid_label = NULL;
  }

  if (code == LV_EVENT_CANCEL) {
    if (lv_obj_is_valid(ui->wifi.keyboard))
      lv_obj_del(ui->wifi.keyboard);
    if (lv_obj_is_valid(ui->wifi.ta_pass))
      lv_obj_del(ui->wifi.ta_pass);
    if (lv_obj_is_valid(ui->wifi.pass_popup))
      lv_obj_del(ui->wifi.pass_popup);

    ui->wifi.keyboard = NULL;
    ui->wifi.ta_pass = NULL;
    ui->wifi.pass_popup = NULL;
    ui->wifi.pass_popup_ssid_label = NULL;
  }
}

void wifi_open_pass_popup(ui_main_menu_t *ui, const char *ssid) {
  if (ui->wifi.pass_popup != NULL && lv_obj_is_valid(ui->wifi.pass_popup))
    return;

  strncpy(ui->wifi.pending_ssid, ssid, sizeof(ui->wifi.pending_ssid) - 1);
  ui->wifi.pending_ssid[sizeof(ui->wifi.pending_ssid) - 1] = '\0';

  lv_obj_t *p = ui->settings.page_content;

  // popup — только заголовок
  lv_obj_t *popup = lv_obj_create(p);
  lv_obj_set_size(popup, 560, 60);
  lv_obj_align(popup, LV_ALIGN_TOP_MID, 0, 30);
  lv_obj_add_style(popup, &ui->style.popup, 0);
  lv_obj_set_style_radius(popup, 12, 0);
  lv_obj_set_style_border_width(popup, 1, 0);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x2D5A8E), 0);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
  ui->wifi.pass_popup = popup;

  // заголовок — название сети
  ui->wifi.pass_popup_ssid_label = lv_label_create(popup);
  lv_obj_add_style(ui->wifi.pass_popup_ssid_label, &ui->font.medium_32, 0);
  lv_label_set_text(ui->wifi.pass_popup_ssid_label, ssid);
  lv_obj_align(ui->wifi.pass_popup_ssid_label, LV_ALIGN_CENTER, 0, 0);

  // поле пароля — на page_content
  ui->wifi.ta_pass = lv_textarea_create(p);
  lv_obj_set_size(ui->wifi.ta_pass, 560, 60);
  lv_obj_align(ui->wifi.ta_pass, LV_ALIGN_TOP_MID, 0, 90);
  lv_textarea_set_placeholder_text(ui->wifi.ta_pass, "Passwort");
  lv_textarea_set_password_mode(ui->wifi.ta_pass, true);
  lv_textarea_set_one_line(ui->wifi.ta_pass, true);

  // клавиатура — на page_content
  ui->wifi.keyboard = lv_keyboard_create(p);
  lv_obj_set_size(ui->wifi.keyboard, lv_obj_get_width(p),
                  lv_obj_get_height(p) / 2);
  lv_obj_align(ui->wifi.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_pass);
  lv_obj_add_event_cb(ui->wifi.keyboard, wifi_pass_popup_connect_cb,
                      LV_EVENT_ALL, ui);
}

// ---------------------------------------------------------------------------
// Scan list — tap on AP
// ---------------------------------------------------------------------------

void wifi_ap_item_click_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_t *btn = lv_event_get_target(e);
  const char *btn_text = lv_list_get_btn_text(ui->wifi.scan_list, btn);
  if (!btn_text || strlen(btn_text) == 0)
    return;

  // пропускаем префикс "[N] "
  const char *ssid = btn_text;
  if (btn_text[0] == '[') {
    const char *p = strchr(btn_text, ' ');
    if (p)
      ssid = p + 1;
  }

  ESP_LOGI(TAG, "AP clicked, ssid='%s'", ssid);

  bool open = true;
  for (int i = 0; i < ui->wifi.scan_count; i++) {
    if (strcmp((char *)ui->wifi.scan_results[i].ssid, ssid) == 0) {
      open = (ui->wifi.scan_results[i].authmode == WIFI_AUTH_OPEN);
      break;
    }
  }

  ESP_LOGI(TAG, "open=%d", open);

  if (open) {
    strncpy(ui->wifi.pending_ssid, ssid, sizeof(ui->wifi.pending_ssid) - 1);
    wifi_connect(ui->wifi.pending_ssid, "");
    if (lv_obj_is_valid(ui->wifi.connected_ssid_label))
      lv_label_set_text(ui->wifi.connected_ssid_label, "verbinde...");
    lv_timer_t *t = lv_timer_create(wifi_check_timer_cb, 500, ui);
    lv_timer_set_repeat_count(t, 10);
  } else {
    wifi_open_pass_popup(ui, ssid);
  }
}

// ---------------------------------------------------------------------------
// Scan timer
// ---------------------------------------------------------------------------
void wifi_manual_keyboard_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_READY) {
    // если фокус на SSID — переключаем на пароль
    if (lv_keyboard_get_textarea(ui->wifi.keyboard) ==
        ui->wifi.pass_popup_ssid_label) {
      lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_pass);
      return;
    }

    // фокус на пароле — подключаемся
    const char *ssid = lv_textarea_get_text(ui->wifi.pass_popup_ssid_label);
    const char *pass = lv_textarea_get_text(ui->wifi.ta_pass);

    if (!ssid || strlen(ssid) < 1)
      return;

    strncpy(ui->wifi.pending_ssid, ssid, sizeof(ui->wifi.pending_ssid) - 1);
    wifi_connect(ui->wifi.pending_ssid, pass);

    if (lv_obj_is_valid(ui->wifi.connected_ssid_label))
      lv_label_set_text(ui->wifi.connected_ssid_label, "verbinde...");

    lv_timer_t *t = lv_timer_create(wifi_check_timer_cb, 500, ui);
    lv_timer_set_repeat_count(t, 10);

    if (lv_obj_is_valid(ui->wifi.keyboard))
      lv_obj_del(ui->wifi.keyboard);
    if (lv_obj_is_valid(ui->wifi.pass_popup))
      lv_obj_del(ui->wifi.pass_popup);

    ui->wifi.pass_popup = NULL;
    ui->wifi.pass_popup_ssid_label = NULL;
    ui->wifi.ta_pass = NULL;
    ui->wifi.keyboard = NULL;
  }

  if (code == LV_EVENT_CANCEL) {
    if (lv_obj_is_valid(ui->wifi.keyboard))
      lv_obj_del(ui->wifi.keyboard);
    if (lv_obj_is_valid(ui->wifi.pass_popup))
      lv_obj_del(ui->wifi.pass_popup);

    ui->wifi.pass_popup = NULL;
    ui->wifi.pass_popup_ssid_label = NULL;
    ui->wifi.ta_pass = NULL;
    ui->wifi.keyboard = NULL;
  }
}

void wifi_manual_ta_focus_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;
  if (!lv_obj_is_valid(ui->wifi.keyboard))
    return;

  lv_obj_t *ta = lv_event_get_target(e);
  lv_keyboard_set_textarea(ui->wifi.keyboard, ta);
}

void wifi_manual_entry_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // guard от двойного открытия
  if (ui->wifi.pass_popup != NULL && lv_obj_is_valid(ui->wifi.pass_popup))
    return;

  lv_obj_t *p = ui->settings.page_content;

  lv_obj_t *popup = lv_obj_create(p);
  lv_obj_set_size(popup, 560, 160);
  lv_obj_align(popup, LV_ALIGN_TOP_MID, 0, 5);
  lv_obj_add_style(popup, &ui->style.popup, 0);
  lv_obj_set_style_radius(popup, 12, 0);
  lv_obj_set_style_border_width(popup, 1, 0);
  lv_obj_set_style_border_color(popup, lv_color_hex(0x2D5A8E), 0);
  lv_obj_clear_flag(popup, LV_OBJ_FLAG_SCROLLABLE);
  ui->wifi.pass_popup = popup;

  // поле SSID
  ui->wifi.pass_popup_ssid_label = lv_textarea_create(popup);
  lv_obj_set_size(ui->wifi.pass_popup_ssid_label, 520, 45);
  lv_obj_align(ui->wifi.pass_popup_ssid_label, LV_ALIGN_TOP_MID, 0, -10);
  lv_textarea_set_placeholder_text(ui->wifi.pass_popup_ssid_label,
                                   "Netzwerkname (SSID)");
  lv_textarea_set_one_line(ui->wifi.pass_popup_ssid_label, true);

  // поле пароля
  ui->wifi.ta_pass = lv_textarea_create(popup);
  lv_obj_set_size(ui->wifi.ta_pass, 520, 45);
  lv_obj_align(ui->wifi.ta_pass, LV_ALIGN_BOTTOM_MID, 0, 10);
  lv_textarea_set_placeholder_text(ui->wifi.ta_pass, "Passwort");
  lv_textarea_set_password_mode(ui->wifi.ta_pass, true);
  lv_textarea_set_one_line(ui->wifi.ta_pass, true);

  lv_obj_add_event_cb(ui->wifi.ta_pass, wifi_manual_ta_focus_cb,
                      LV_EVENT_CLICKED, ui);
  lv_obj_add_event_cb(ui->wifi.pass_popup_ssid_label, wifi_manual_ta_focus_cb,
                      LV_EVENT_CLICKED, ui);

  // клавиатура
  ui->wifi.keyboard = lv_keyboard_create(p);
  lv_obj_set_size(ui->wifi.keyboard, lv_obj_get_width(p),
                  lv_obj_get_height(p) / 2);
  lv_obj_align(ui->wifi.keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.pass_popup_ssid_label);
  lv_obj_add_event_cb(ui->wifi.keyboard, wifi_manual_keyboard_cb, LV_EVENT_ALL,
                      ui);
}

void wifi_refresh_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (!lv_obj_is_valid(ui->wifi.scan_status_label))
    return;
  lv_label_set_text(ui->wifi.scan_status_label, "Suche...");

  lv_obj_clean(ui->wifi.scan_list);

  if (get_wifi_status() != WIFI_CONNECTED) {
    wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(300));
  }

  wifi_scan_start();
  lv_timer_t *t = lv_timer_create(wifi_scan_timer_cb, 500, ui);
  lv_timer_set_repeat_count(t, 40);
}

void wifi_scan_timer_cb(lv_timer_t *timer) {

  ui_main_menu_t *ui = (ui_main_menu_t *)timer->user_data;
  if (!ui)
    return;
  ESP_LOGI(TAG, "scan timer tick, done=%d", wifi_scan_is_done());
  if (!wifi_scan_is_done())
    return;

  lv_timer_del(timer);

  ui->wifi.scan_count = WIFI_SCAN_MAX_AP;
  wifi_scan_get_results(ui->wifi.scan_results, &ui->wifi.scan_count);

  // очищаем список
  if (!lv_obj_is_valid(ui->wifi.scan_list))
    return;
  lv_obj_clean(ui->wifi.scan_list);
  // кнопка обновления — всегда первая
  lv_obj_t *refresh_btn = lv_list_add_btn(ui->wifi.scan_list, LV_SYMBOL_REFRESH,
                                          "Suche wiederholen");
  lv_obj_add_flag(refresh_btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(refresh_btn, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(refresh_btn, lv_color_hex(0x60A5FA), 0);
  lv_obj_add_event_cb(refresh_btn, wifi_refresh_cb, LV_EVENT_SHORT_CLICKED, ui);

  if (ui->wifi.scan_count == 0) {
    if (lv_obj_is_valid(ui->wifi.scan_status_label))
      lv_label_set_text(ui->wifi.scan_status_label, "Keine Netze gefunden");
    // добавляем кнопку и выходим
    lv_obj_t *manual_btn =
        lv_list_add_btn(ui->wifi.scan_list, LV_SYMBOL_EDIT, "Manuell eingeben");
    lv_obj_add_flag(manual_btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_text_font(manual_btn, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(manual_btn, lv_color_hex(0x60A5FA), 0);
    lv_obj_add_event_cb(manual_btn, wifi_manual_entry_cb,
                        LV_EVENT_SHORT_CLICKED, ui);
    return;
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "%d Netze gefunden", ui->wifi.scan_count);
  if (lv_obj_is_valid(ui->wifi.scan_status_label))
    lv_label_set_text(ui->wifi.scan_status_label, buf);

  for (int i = 0; i < ui->wifi.scan_count; i++) {
    uint8_t bars = rssi_to_bars(ui->wifi.scan_results[i].rssi);

    // формируем строку: "████░░  NetworkName"
    char bars_str[8];
    snprintf(bars_str, sizeof(bars_str), "[%d] ", bars);

    char label[64];
    snprintf(label, sizeof(label), "%s%s", bars_str,
             (char *)ui->wifi.scan_results[i].ssid);

    lv_obj_t *btn = lv_list_add_btn(ui->wifi.scan_list, NULL, label);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(btn, wifi_ap_item_click_cb, LV_EVENT_SHORT_CLICKED, ui);
    lv_obj_set_style_text_font(btn, &lv_font_montserrat_20, 0);
    ESP_LOGI(TAG, "added btn: %s", label);
  }
  // кнопка ручного ввода
  lv_obj_t *manual_btn =
      lv_list_add_btn(ui->wifi.scan_list, LV_SYMBOL_EDIT, "Manuell eingeben");
  lv_obj_add_flag(manual_btn, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_text_font(manual_btn, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(manual_btn, lv_color_hex(0x60A5FA), 0);
  lv_obj_add_event_cb(manual_btn, wifi_manual_entry_cb, LV_EVENT_SHORT_CLICKED,
                      ui);
}
/////////////////////////////////////////////////wifi events
// Таймер который проверяет статус каждые 500мс, максимум 10 раз (5 секунд)
void wifi_check_timer_cb(lv_timer_t *timer) {
  ui_main_menu_t *ui = (ui_main_menu_t *)timer->user_data;
  static uint8_t attempts = 0;

  attempts++;

  if (get_wifi_status() == WIFI_CONNECTED) {
    if (lv_obj_is_valid(ui->wifi.connected_ssid_label)) {
      const char *ssid = get_wifi_ssid();
      lv_label_set_text(ui->wifi.connected_ssid_label,
                        ssid ? ssid : LV_SYMBOL_OK " verbunden");
      wifi_update_signal_bars(ui, rssi_to_bars(get_wifi_rssi()));
    }
    attempts = 0;
    lv_timer_del(timer);
    return;
  }

  if (attempts >= 10) {
    if (lv_obj_is_valid(ui->wifi.connected_ssid_label))
      lv_label_set_text(ui->wifi.connected_ssid_label,
                        LV_SYMBOL_CLOSE " Fehler");
    attempts = 0;
    lv_timer_del(timer);
  }
}
