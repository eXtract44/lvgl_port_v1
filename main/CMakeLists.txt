idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" "main.c" "lvgl_port.c" 
    
    "user/periphery/wifi_user.c"
    "user/periphery/open_meteo.c"
    "user/periphery/time_user.c"
    "user/periphery/sd_card.c"
     "user/periphery/nvs_user.c"
     "user/periphery/i2c_bus.c"
     "user/periphery/sensors.c" 
     "user/periphery/backlight.c"
     "user/periphery/ota.c"
    
    "user/menu/ui_core.c" 
    "user/menu/ui_blocks.c" 
    "user/menu/ui_popup_settings.c" 
    "user/menu/ui_theme.c" 
    "user/menu/ui_timers.c" 
    "user/menu/ui_weather_anim.c" 
    "user/menu/ui_widgets.c" 
    "user/menu/ui_wifi.c" 
    "user/menu/ui_time.c" 
    
    "user/fonts/my_symbols.c" 
    "user/fonts/my_time_font.c"
   
    "user/pics/cloud_big_110_50.c"
    "user/pics/cloud_mid_90_45.c"
    "user/pics/cloud_small_70_35.c"
    "user/pics/cloud_thin_80_30.c"
    "user/pics/moon_42_42.c"
    "user/pics/sun_48_48.c"
    "user/pics/rain_drop_heavy_9_22.c"
    "user/pics/snow_flake_2_15_15.c"   
    "user/pics/wind_60_50.c"
      
    INCLUDE_DIRS "."

    REQUIRES esp_https_ota app_update esp_wifi nvs_flash fatfs json mbedtls)

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
