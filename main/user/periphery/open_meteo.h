/*
 * open_meteo.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_OPEN_METEO_H_
#define MAIN_USER_PERIPHERY_OPEN_METEO_H_

#include <stdint.h>
#include <stdbool.h>

#include "sensors.h"

typedef struct {
    int8_t   temp_max;      // °C
    int8_t   temp_min;      // °C
    uint8_t  humidity_max;  // %
    uint8_t  weathercode;   // WMO код
    uint32_t sunrise;       // unix time
    uint32_t sunset;        // unix time
} forecast_day_t;

#define FORECAST_DAYS 3

typedef struct {
    forecast_day_t day[FORECAST_DAYS];
    bool valid;  // данные получены
} forecast_data_t;



typedef enum {
    // Крупные города
    CITY_BERLIN = 0,
    CITY_HAMBURG,
    CITY_MUNICH,
    CITY_COLOGNE,
    CITY_FRANKFURT,
    CITY_STUTTGART,
    CITY_DUESSELDORF,
    CITY_LEIPZIG,
    CITY_ESSEN,
    CITY_BREMEN,
    CITY_DRESDEN,
    CITY_HANNOVER,
    CITY_NUREMBERG,
    CITY_DUISBURG,
    CITY_BOCHUM,
    CITY_WUPPERTAL,
    CITY_BIELEFELD,
    CITY_BONN,
    CITY_MUENSTER,
    CITY_KARLSRUHE,
    CITY_MANNHEIM,
    CITY_AUGSBURG,
    CITY_WIESBADEN,
    CITY_GELSENKIRCHEN,
    CITY_MOENCHENGLADBACH,
    CITY_BRAUNSCHWEIG,
    CITY_CHEMNITZ,
    CITY_KIEL,
    CITY_AACHEN,
    CITY_HALLE,
    CITY_MAGDEBURG,
    CITY_FREIBURG,
    CITY_KREFELD,
    CITY_LUEBECK,
    CITY_OBERHAUSEN,
    CITY_ERFURT,
    CITY_MAINZ,
    CITY_ROSTOCK,
    CITY_KASSEL,

    // Дортмунд — районы
    CITY_DO_MITTE,
    CITY_DO_EVING,
    CITY_DO_SCHARNHORST,
    CITY_DO_BRACKEL,
    CITY_DO_APLERBECK,
    CITY_DO_HOERDE,
    CITY_DO_HOMBRUCH,
    CITY_DO_LUETGENDORTMUND,
    CITY_DO_HUCKARDE,
    CITY_DO_MENGEDE,
    CITY_DO_INNENSTADT_W,
    CITY_DO_INNENSTADT_O,
    CITY_DO_INNENSTADT_N,
    CITY_DO_BORSIGPLATZ,
    CITY_DO_DORSTFELD,
    CITY_DO_MARTEN,
    CITY_DO_KIRCHLINDE,
    CITY_DO_DERNE,
    CITY_DO_GREVEL,
    CITY_DO_ALT_SCHARNHORST,
    CITY_DO_ASSELN,
    CITY_DO_WAMBEL,
    CITY_DO_WICKEDE,
    CITY_DO_BERGHOFEN,
    CITY_DO_KIRCHHOERDE,
    CITY_DO_BAROP,
    CITY_DO_EICHLINGHOFEN,
    CITY_DO_BODELSCHWINGH,
    CITY_DO_BRECHTEN,
    CITY_DO_LINDENHORST,
    CITY_DO_HUSEN,
    CITY_DO_KOERNE,
    CITY_DO_OESPEL,
    CITY_COUNT
} city_id_t;

 typedef struct {
    const char *name;
    float lat;
    float lon;
} city_t;

typedef struct {
  double temperature_2m;
  int relative_humidity_2m;
  double snow;
  double rain;
  int cloud_cover;
  double wind_speed_10m;
  int is_day;
    // --- новое ---
  double apparent_temperature;    // ощущаемая температура снаружи
  double uv_index;                // УФ индекс
  double precipitation_probability; // вероятность осадков %
  double surface_pressure;        // давление гПа

} current_weather_t;

void fetch_weather(void);
void build_weather_url(int city_index);
const char* weathercode_to_text(uint8_t code);

float get_weather_temperature();
uint8_t get_weather_humidity();
uint8_t get_weather_wind();
uint8_t get_weather_clouds();
float get_weather_rain();
float get_weather_snow();
uint8_t get_is_day();

double get_apparent_temperature(void);
double get_uv_index(void) ;
double get_precipitation_probability(void) ;
double get_surface_pressure(void);



#endif /* MAIN_USER_PERIPHERY_OPEN_METEO_H_ */
