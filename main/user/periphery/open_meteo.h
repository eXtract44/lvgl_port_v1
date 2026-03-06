/*
 * open_meteo.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_OPEN_METEO_H_
#define MAIN_USER_PERIPHERY_OPEN_METEO_H_

typedef enum {
    CITY_BERLIN = 0,
    CITY_HAMBURG,
    CITY_MUNICH,
    CITY_COLOGNE,
    CITY_FRANKFURT,
    CITY_STUTTGART,
    CITY_DUESSELDORF,
    CITY_LEIPZIG,
    CITY_DORTMUND,
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

    CITY_COUNT
} city_id_t;
 typedef struct {
    const char *name;
    float lat;
    float lon;
} city_t;

void fetch_weather(void);
void build_weather_url(int city_index);



#endif /* MAIN_USER_PERIPHERY_OPEN_METEO_H_ */
