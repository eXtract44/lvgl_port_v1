idf_component_register(
    SRCS "waveshare_rgb_lcd_port.c" "main.c" "lvgl_port.c" 
    
    "user/periphery/wifi.c"
    "user/periphery/open_meteo.c"
    "user/periphery/time.c"
    "user/periphery/sd_card.c"
    
    "user/menu/lvgl_menu.c" 
    "user/periphery/periphery.c" 
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
      
    INCLUDE_DIRS ".")

idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)
