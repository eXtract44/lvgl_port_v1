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
    CITY_COUNT
} city_id_t;

 typedef struct {
    const char *name;
    float lat;
    float lon;
} city_t;

void fetch_weather(void);
void build_weather_url(int city_index);
const char* weathercode_to_text(uint8_t code);



#endif /* MAIN_USER_PERIPHERY_OPEN_METEO_H_ */
