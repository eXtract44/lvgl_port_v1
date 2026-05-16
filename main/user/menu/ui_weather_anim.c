

#include "ui_weather_anim.h"
#include "ui_core.h"
#include "wifi_user.h"



void anim_sun_moon_orbit(void *var, int32_t angle) {
  lv_obj_t *obj = var;

  const int cx = 60; // центр области 250x225
  const int cy = 60;
  const int r = 20;

  float rad = angle * 3.14159f / 1800.0f; // 0..3600

  lv_coord_t x = cx + r * cosf(rad);
  lv_coord_t y = cy - r * sinf(rad);

  lv_obj_align(obj, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
               x + BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
               y + BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);
}

lv_obj_t *create_anim_image_orbit(const lv_img_dsc_t *img_src,
                                         lv_obj_t *parent, lv_align_t align,
                                         lv_coord_t x_ofs, lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_anim_image_orbit");
    // return;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_align(img, align, x_ofs, y_ofs);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, img);
  lv_anim_set_exec_cb(&a, anim_sun_moon_orbit);
  lv_anim_set_values(&a, 0, 3600);
  lv_anim_set_time(&a, BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_SUN_MOON);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

void cloud_anim_cb(void *var, int32_t v) {
  cloud_anim_t *c = (cloud_anim_t *)var;
  // [FIX D] Проверка перед lv_obj_set_pos
  if (!c || !c->obj || !lv_obj_is_valid(c->obj))
    return;

  int32_t x = c->base_x + (lv_trigo_sin(v) * c->amplitude) / LV_TRIGO_SIN_MAX;
  lv_obj_set_pos(c->obj, x, c->base_y);
}

lv_obj_t *create_cloud_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                            lv_coord_t x_ofs, lv_coord_t y_ofs,
                            int16_t amplitude, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_cloud_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_set_pos(img, x_ofs, y_ofs);

  static cloud_anim_t cloud_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_CLOUDS];
  static uint8_t pool_index = 0;

  // [FIX A] Проверка переполнения — раньше этой строки не было!
  if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_CLOUDS)
    pool_index = 0;

  cloud_anim_t *c = &cloud_pool[pool_index++];
  c->obj = img;
  c->base_x = x_ofs;
  c->base_y = y_ofs;
  c->amplitude = amplitude;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, c);
  lv_anim_set_exec_cb(&a, cloud_anim_cb);
  lv_anim_set_values(&a, 0, 360);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

void wind_anim_cb(void *var, int32_t v) {
  wind_anim_t *w = (wind_anim_t *)var;
  if (!w || !w->obj || !lv_obj_is_valid(w->obj))
    return;

  if (w->container_w < 1)
    w->container_w = 1;

  // [FIX #2] Правильный диапазон для lv_trigo_sin: 0..3600
  // v идёт от -img_w до container_w+img_w
  // нормализуем в 0..3600 для синуса
  int32_t sin_arg = ((v + w->img_w) * 3600) / (w->container_w + w->img_w * 2);
  sin_arg = sin_arg % 3600;

  lv_coord_t x =
      v; // [FIX #3] x = v напрямую, диапазон уже с отрицательным стартом
  lv_coord_t y = w->base_y +
                 (lv_trigo_sin(sin_arg) * w->turbulence) / LV_TRIGO_SIN_MAX / 3;

  lv_obj_set_pos(w->obj, x, y);
}

lv_obj_t *create_wind_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           lv_coord_t x_ofs, lv_coord_t y_ofs,
                           int16_t turbulence, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_wind_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_update_layout(parent); // ← форсируе

  // [FIX #1] Снимаем авто-позиционирование
  lv_obj_clear_flag(img, LV_OBJ_FLAG_SNAPPABLE);
  lv_obj_set_pos(img, x_ofs, y_ofs);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t img_w = lv_obj_get_width(img);

  static wind_anim_t wind_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS];
  static uint8_t pool_index = 0;

  if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS)
    pool_index = 0;

  wind_anim_t *w = &wind_pool[pool_index++];
  w->obj = img;
  w->base_y = y_ofs;
  w->container_w = container_w;
  w->img_w = img_w;
  w->turbulence = turbulence;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, w);
  lv_anim_set_exec_cb(&a, wind_anim_cb);
  // [FIX #3] Старт с -img_w (левее контейнера), конец за правым краем
  lv_anim_set_values(&a, -img_w, container_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

void rain_snow_anim_cb(void *var, int32_t v) {
  rain_snow_anim_t *r = (rain_snow_anim_t *)var;
  // [FIX D] Проверка перед lv_obj_set_pos
  if (!r || !r->obj || !lv_obj_is_valid(r->obj))
    return;

  int32_t value = v + r->phase;
  lv_coord_t range_x = r->container_w + r->img_w;
  lv_coord_t range_y = r->container_h + r->img_h;

  if (range_x == 0 || range_y == 0)
    return;

  lv_coord_t x = (value % range_x) - r->img_w;
  lv_coord_t y = ((value * r->slope) % range_y) - r->img_h;
  lv_obj_set_pos(r->obj, x, y + 30);
}

lv_obj_t *create_rain_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_rain_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t container_h = lv_obj_get_height(parent);
  lv_coord_t img_w = lv_obj_get_width(img);
  lv_coord_t img_h = lv_obj_get_height(img);

  // [FIX B] Отдельный пул только для дождя
  static rain_snow_anim_t rain_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS];
  static uint8_t rain_pool_index = 0;

  if (rain_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS)
    rain_pool_index = 0;

  rain_snow_anim_t *r = &rain_pool[rain_pool_index++];
  r->obj = img;
  r->container_w = container_w;
  r->container_h = container_h;
  r->img_w = img_w;
  r->img_h = img_h;
  r->slope = slope;
  r->phase = lv_rand(0, container_w);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, r);
  lv_anim_set_exec_cb(&a, rain_snow_anim_cb);
  lv_anim_set_values(&a, 0, container_w + img_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

lv_obj_t *create_snow_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_snow_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t container_h = lv_obj_get_height(parent);
  lv_coord_t img_w = lv_obj_get_width(img);
  lv_coord_t img_h = lv_obj_get_height(img);

  // [FIX B] Отдельный пул только для снега
  static rain_snow_anim_t snow_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];
  static uint8_t snow_pool_index = 0;

  if (snow_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
    snow_pool_index = 0;

  rain_snow_anim_t *r = &snow_pool[snow_pool_index++];
  r->obj = img;
  r->container_w = container_w;
  r->container_h = container_h;
  r->img_w = img_w;
  r->img_h = img_h;
  r->slope = slope;
  r->phase = lv_rand(0, container_w);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, r);
  lv_anim_set_exec_cb(&a, rain_snow_anim_cb);
  lv_anim_set_values(&a, 0, container_w + img_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

void rain_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {
  static uint8_t prev =
      255; // 255 = невалидное значение → форсирует первый update

  if (visible_count == prev)
    return;

  // Клипуем на максимум — защита от выхода за пределы массива
  if (visible_count > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS)
    visible_count = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;

  if (visible_count > prev) {
    for (uint8_t i = prev == 255 ? 0 : prev; i < visible_count; i++) {
      if (ui->animation.rain[i] && lv_obj_is_valid(ui->animation.rain[i]))
        lv_obj_clear_flag(ui->animation.rain[i], LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    for (uint8_t i = visible_count;
         i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS : prev);
         i++) {
      if (ui->animation.rain[i] && lv_obj_is_valid(ui->animation.rain[i]))
        lv_obj_add_flag(ui->animation.rain[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  prev = visible_count;
}

void snow_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {

  static uint8_t prev =
      255; // 255 = невалидное значение → форсирует первый update

  if (visible_count == prev)
    return;

  // Клипуем на максимум — защита от выхода за пределы массива
  if (visible_count > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
    visible_count = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;

  if (visible_count > prev) {
    for (uint8_t i = prev == 255 ? 0 : prev; i < visible_count; i++) {
      if (ui->animation.snow[i] && lv_obj_is_valid(ui->animation.snow[i]))
        lv_obj_clear_flag(ui->animation.snow[i], LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    for (uint8_t i = visible_count;
         i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS : prev);
         i++) {
      if (ui->animation.snow[i] && lv_obj_is_valid(ui->animation.snow[i]))
        lv_obj_add_flag(ui->animation.snow[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  prev = visible_count;
}

void draw_weather_sun_moon(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_SUN
  set_visible(ui->animation.image.sun_48_48, true);
  set_visible(ui->animation.image.moon_42_42, false);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.image.sun_48_48, false);
    set_visible(ui->animation.image.moon_42_42, false);
    prev = 255;
  } else {

    uint8_t is_day = get_is_day();

    if (is_day == prev)
      return;

    if (get_is_day()) {
      set_visible(ui->animation.image.sun_48_48, true);
      set_visible(ui->animation.image.moon_42_42, false);
    } else {
      set_visible(ui->animation.image.sun_48_48, false);
      set_visible(ui->animation.image.moon_42_42, true);
    }
    prev = is_day;
  }

#endif
}

void draw_weather_clouds(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_CLOUD
  set_visible(ui->animation.image.cloud.big_110_50, true);
  set_visible(ui->animation.image.cloud.mid_90_45, true);
  set_visible(ui->animation.image.cloud.small_70_35, true);
  set_visible(ui->animation.image.cloud.thin_80_30, true);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.image.cloud.thin_80_30, false);
    set_visible(ui->animation.image.cloud.small_70_35, false);
    set_visible(ui->animation.image.cloud.mid_90_45, false);
    set_visible(ui->animation.image.cloud.big_110_50, false);
    prev = 255;
  } else {
    uint8_t current_clouds = get_weather_clouds();

    if (current_clouds == prev)
      return;
    if (current_clouds < 15) {
      set_visible(ui->animation.image.cloud.thin_80_30, false);
      set_visible(ui->animation.image.cloud.small_70_35, false);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 15 && current_clouds < 25) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, false);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 25 && current_clouds < 50) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 50 && current_clouds < 75) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, true);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 75) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, true);
      set_visible(ui->animation.image.cloud.big_110_50, true);
    }
    prev = current_clouds;
  }

#endif
}

void draw_weather_wind(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_WIND
  set_visible(ui->animation.wind.slow, true);
  set_visible(ui->animation.wind.med, true);
  set_visible(ui->animation.wind.fast, true);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.wind.slow, false);
    set_visible(ui->animation.wind.med, false);
    set_visible(ui->animation.wind.fast, false);
    prev = 255;
  } else {
    uint8_t current_wind = get_weather_wind();

    if (current_wind == prev)
      return;

    // < 2  м/с — всё выключено
    // 2..9  — только slow
    // 10..19 — slow + med
    // 20+   — все три
    set_visible(ui->animation.wind.slow, current_wind >= 2);
    set_visible(ui->animation.wind.med, current_wind >= 10);
    set_visible(ui->animation.wind.fast, current_wind >= 20);
    prev = current_wind;
  }

#endif
}

void draw_weather_rain(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_RAIN
  rain_set_intensity(10);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    rain_set_intensity(0, ui);
  } else {
    uint8_t current_rain = get_weather_rain();
    if (current_rain > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS) {
      current_rain = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;
    }
    rain_set_intensity(current_rain, ui);
  }

#endif
}

void draw_weather_snow(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_SNOW
  snow_set_intensity(10);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    snow_set_intensity(0, ui);
  } else {
    uint8_t current_snow = get_weather_snow();
    current_snow =
        current_snow * BLOCK_BOT_RIGHT_MULT_FACTOR_WEATHER_ANIM_SNOWS;
    if (current_snow > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS) {
      current_snow = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;
    }
    snow_set_intensity(current_snow, ui);
  }
#endif
}

void draw_weather(ui_main_menu_t *ui) {
#if ACTIVATE_ANIM_SUN_MOON
  draw_weather_sun_moon(ui);
#endif
#if ACTIVATE_ANIM_CLOUD
  draw_weather_clouds(ui);
#endif
#if ACTIVATE_ANIM_WIND
  draw_weather_wind(ui);
#endif
#if ACTIVATE_ANIM_RAIN
  draw_weather_rain(ui);
#endif
#if ACTIVATE_ANIM_SNOW
  draw_weather_snow(ui);
#endif
}