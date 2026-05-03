#ifndef ESPNOW_H
#define ESPNOW_H

#include <stdbool.h>
#include <stdio.h>

#include "esp_now.h"

#define MSG_SYNC 1
#define MSG_DATA 2
#define SLOT_MS 2000
#define NORMAL_CHANNEL 1
#define CYCLE_US \
  (30LL * 60 * 1000000LL + 30000000LL)  // 30 minutes + 30 secondes

typedef struct __attribute__((packed)) {
  uint8_t type;
  uint8_t uid;
  int16_t weight_x10;
  int16_t temp_x10;
  uint16_t battery_mv;
  int16_t station_temp_x10;
  uint16_t station_humidity_x100;
  uint16_t station_battery_mv;
} client_msg_t;

typedef struct {
  uint8_t type;
  int64_t cycle_us;
} sync_msg_t;

void init_wifi(void);
void init_espnow(void);
void on_data_recv(const esp_now_recv_info_t* info, const uint8_t* data,
                  int len);
void on_data_sent(const esp_now_send_info_t* info,
                  esp_now_send_status_t status);
void send_adv(uint8_t dip_switch_value, float temperature, float humidity,
              float voltage);
void sendData(const client_msg_t* data);
#endif  // ESPNOW_H