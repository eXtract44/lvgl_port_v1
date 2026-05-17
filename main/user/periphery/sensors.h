/*
 * periphery.h
 *
 *  Created on: 07.02.2026
 *      Author: toose
 *
 *  SHT31 → SHT41 migration
 */

#ifndef MAIN_PERIPHERY_H_
#define MAIN_PERIPHERY_H_

#include <stdint.h>

// ---------------------------------------------------------------------------
// SHT41
// ---------------------------------------------------------------------------
// I2C address (fixed, no ADDR pin on SHT41)
#define SHT41_ADDRESS           0x44

// Single-byte commands
#define SHT41_CMD_SOFTRESET     0x94   // soft reset (~1 ms)

// Measure commands (single-shot, no clock stretching)
#define SHT41_CMD_MEAS_HIGH     0xFD   // High precision   ~10 ms
#define SHT41_CMD_MEAS_MED      0xF6   // Medium precision  ~5 ms
#define SHT41_CMD_MEAS_LOW      0xE0   // Low precision     ~2 ms

// Use High precision by default
#define SHT41_CMD_MEAS          SHT41_CMD_MEAS_HIGH
#define SHT41_MEAS_DELAY_US     10000  // 10 ms for High precision

typedef enum {
    SHT41_STATE_IDLE = 0,
    SHT41_STATE_TRIGGERED,
} sht41_state_t;

typedef enum {
    SHT41_STATE_FAIL,
    SHT41_STATE_OK,
} sht41_life_t;

typedef struct {
    float         temperature;
    uint8_t       humidity;
    sht41_state_t state;
    sht41_life_t  life;
    int64_t       trigger_time;
} sht41_data_t;

void    sht41_init(void);
void    sht41_read(void);
float   get_temperature_sht41(void);
uint8_t get_humidity_sht41(void);

// ---------------------------------------------------------------------------
// SGP30  (unchanged)
// ---------------------------------------------------------------------------
#define CRC8_POLYNOMIAL     0x31
#define SGP30_ADDR          0x58
#define SGP30_ADDR_WRITE    SGP30_ADDR
#define SGP30_ADDR_READ     SGP30_ADDR

typedef enum {
    SGP30_STATE_FAIL,
    SGP30_STATE_OK,
} sgp30_state_t;

typedef struct sgp30_data_st {
    uint16_t      co2;
    uint16_t      tvoc;
    sgp30_state_t state;
    uint8_t init_ok;
} sgp30_data_t;

typedef enum sgp30_cmd_en {
    INIT_AIR_QUALITY    = 0x2003,
    MEASURE_AIR_QUALITY = 0x2008
} sgp30_cmd_t;

int      sgp30_init(void);
int      sgp30_read(void);
uint16_t get_co2_sgp30(void);
uint16_t get_tvoc_sgp30(void);

void sgp30_set_humidity_compensation(float temperature_c, float relative_humidity_pct);

// ---------------------------------------------------------------------------
// Aggregated read
// ---------------------------------------------------------------------------
void read_sensors(void);


#endif /* MAIN_PERIPHERY_H_ */
