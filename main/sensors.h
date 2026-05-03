#ifndef SENSORS_H
#define SENSORS_H

#include <stdbool.h>
#include <stdint.h>

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_FREQ_HZ 100000
#define BME280_I2C_ADDR 0x76

#define ADC_UNIT_ID ADC_UNIT_1
#define ADC_ATTEN ADC_ATTEN_DB_12  // jusqu'à ~3.3V

void init_sensors(void);

float read_temperature(void);
float read_humidity(void);
float read_voltage(void);

bool is_sigfox_enabled(void);
bool is_lora_enabled(void);
bool is_setup_enabled(void);

uint8_t read_nodes_dip_switch(void);
uint8_t read_address_dip_switch(void);

void disable_inputs(void);

#endif  // SENSORS_H