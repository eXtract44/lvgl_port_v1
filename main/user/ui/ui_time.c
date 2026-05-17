#include "ui_time.h"
#include "ui_core.h"
#include "wifi_user.h"

void print_wday(uint8_t wday, ui_main_menu_t *ui) {
  // При отсутствии WiFi прячем highlight
  if (wday == WDAY_KEIN_WLAN) {
    if (ui->time.wday_highlight)
      lv_obj_add_flag(ui->time.wday_highlight, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (wday >= 7) {
    ESP_LOGE(TAG, "ERROR print_wday: invalid wday=%d", wday);
    return;
  }
  if (ui->time.wday_labels[wday] == NULL || ui->time.wday_highlight == NULL) {
    ESP_LOGE(TAG, "ERROR print_wday: null ptr");
    return;
  }
  // показываем highlight и перемещаем к текущему дню
  static const uint8_t wday_to_idx[7] = {6, 0, 1, 2, 3, 4, 5}; // So→6, Mo→0
  lv_obj_clear_flag(ui->time.wday_highlight, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align_to(ui->time.wday_highlight,
                  ui->time.wday_labels[wday_to_idx[wday]], LV_ALIGN_CENTER, 0,
                  0);
}

void print_time(uint8_t time_hour, uint8_t time_minute, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.hour_minute_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_time");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (time_hour < 10 && time_minute < 10) {
      sprintf(ui->string_buffer, "0%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour > 9 && time_minute < 10) {
      sprintf(ui->string_buffer, "%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour < 10 && time_minute > 9) {
      sprintf(ui->string_buffer, "0%d:%d", (int)time_hour, (int)time_minute);
    } else {
      sprintf(ui->string_buffer, "%d:%d", (int)time_hour, (int)time_minute);
    }
  } else {
    sprintf(ui->string_buffer, "0%d:0%d", (int)0, 0);
  }

  lv_label_set_text(parent, ui->string_buffer);
}

void print_mday(uint8_t date_day, uint8_t date_month, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.mday_month_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_mday");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (date_day < 10 && date_month < 10) {
      sprintf(ui->string_buffer, "0%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day > 9 && date_month < 10) {
      sprintf(ui->string_buffer, "%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day < 10 && date_month > 9) {
      sprintf(ui->string_buffer, "0%d.%d", (int)date_day, (int)date_month);
    } else {
      sprintf(ui->string_buffer, "%d.%d", (int)date_day, (int)date_month);
    }
  } else {
    sprintf(ui->string_buffer, "0%d.0%d", 0, 0);
  }
  lv_label_set_text(parent, ui->string_buffer);
}

// Алгоритм Гаусса — возвращает день и месяц Пасхи для заданного года
void get_easter(uint16_t year, uint8_t *out_day, uint8_t *out_month) {
  int a = year % 19;
  int b = year % 4;
  int c = year % 7;
  int k = year / 100;
  int p = (13 + 8 * k) / 25;
  int q = k / 4;
  int M = (15 - p + k - q) % 30;
  int N = (4 + k - q) % 7;
  int d = (19 * a + M) % 30;
  int e = (2 * b + 4 * c + 6 * d + N) % 7;
  int day = 22 + d + e;
  int month = 3;
  if (day > 31) {
    day -= 31;
    month = 4;
    // исключения алгоритма
    if (day == 26)
      day = 19;
    if (day == 25 && d == 28 && e == 6 && a > 10)
      day = 18;
  }
  *out_day = (uint8_t)day;
  *out_month = (uint8_t)month;
}

// Возвращает название федерального праздника Германии или NULL
const char *get_german_holiday(uint8_t day, uint8_t month, uint16_t year) {
  // Фиксированные праздники
  if (day == 1 && month == 1)
    return "Neujahr"; // 7  ✓
  if (day == 1 && month == 5)
    return "Tag d. Arbeit"; // 13 ✗ — "Maifeiertag"? (12 ✓)
  if (day == 3 && month == 10)
    return "Tag d. Einheit"; // 14 ✗ — "Dt. Einheit"? (11 ✓)
  if (day == 25 && month == 12)
    return "1. Weihnacht"; // 11 ✓
  if (day == 26 && month == 12)
    return "2. Weihnacht"; // 11 ✓

  // Праздники относительно Пасхи
  uint8_t e_day, e_month;
  get_easter(year, &e_day, &e_month);

  // Для расчёта смещений переводим Пасху в день года
  // Используем простой сдвиг через дату: вычитаем/прибавляем дни
  // Проще: сравниваем день+месяц с Пасхой ± смещение
  // Функция: easter_offset → проверяем совпадение
  // Считаем день года для Пасхи и для проверяемой даты
  static const uint8_t days_in_month[13] = {0,  31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
  uint16_t easter_doy = e_day;
  for (int m = 1; m < e_month; m++)
    easter_doy += days_in_month[m];

  uint16_t check_doy = day;
  for (int m = 1; m < month; m++)
    check_doy += days_in_month[m];

  int16_t diff = (int16_t)check_doy - (int16_t)easter_doy;

  if (diff == -2)
    return "Karfreitag"; // 10 ✓
  if (diff == 0)
    return "Ostersonntag"; // 12 ✓
  if (diff == 1)
    return "Ostermontag"; // 11 ✓
  if (diff == 39)
    return "Himmelfahrt"; // 11 ✓
  if (diff == 49)
    return "Pfingstsonntag"; // 14 ✗ — "Pfingstso."?  (10 ✓)
  if (diff == 50)
    return "Pfingstmontag"; // 13 ✗ — "Pfingstmo."?  (10 ✓)
  if (diff == 60)
    return "Fronleichnam"; // 12 ✓

  return NULL;
}

void print_holiday(uint8_t day, uint8_t month, uint16_t year,
                   ui_main_menu_t *ui) {
  if (ui->time.holiday_label == NULL) {
    ESP_LOGE(TAG, "ERROR print_holiday: null ptr");
    return;
  }
  const char *holiday = get_german_holiday(day, month, year);
  if (holiday == NULL) {
    lv_obj_add_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text_fmt(ui->time.holiday_label, MY_BELL_SYMBOL " %s",
                          holiday);
    lv_obj_clear_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  }
}