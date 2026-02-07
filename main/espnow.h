#ifndef ESPNOW_H
#define ESPNOW_H

#include <stdbool.h>
#include <stdio.h>

#include "esp_now.h"

#define NB_SLOTS 16
#define MSG_SYNC 1
#define MSG_DATA 2
#define SLOT_MS 2000
#define NORMAL_CHANNEL 1
#define CYCLE_US (30LL * 60 * 1000000LL)

typedef struct {
  uint8_t type;
  uint8_t uid;
  float weight;
  float temperature;
  float battery;
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
void send_adv();
#endif  // ESPNOW_H