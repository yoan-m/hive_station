#include "espnow.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

static const char* TAG = "ESPNOW";
#define NVS_NAMESPACE "espnow"
uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void init_wifi(void) {
  ESP_LOGI(TAG, "Initialisation WiFi...");
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
  ESP_ERROR_CHECK(esp_wifi_set_channel(NORMAL_CHANNEL, WIFI_SECOND_CHAN_NONE));
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  ESP_LOGI(TAG, "MAC %02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2],
           mac[3], mac[4], mac[5]);
}

void on_data_sent(const esp_now_send_info_t* info,
                  esp_now_send_status_t status) {
  ESP_LOGI(TAG, "Message envoyé: %s",
           status == ESP_NOW_SEND_SUCCESS ? "OK" : "FAIL");
}

void on_data_recv(const esp_now_recv_info_t* info, const uint8_t* data,
                  int len) {
  ESP_LOGI(TAG, "Message reçu");
  /*ESP_LOGI(TAG, "Message reçu de %02X:%02X:%02X:%02X:%02X:%02X, len=%d",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5], len);
  if (data[0] != MSG_DATA) return;

  client_msg_t msg;
  memcpy(&msg, data, sizeof(msg));
  ESP_LOGI(TAG, "UID %d | %.2f kg | %.2f C | %.2f V\n", msg.uid, msg.weight,
           msg.temperature, msg.battery);*/
  /*
    server_msg_t reply = {.uid = msg.uid, .type = 2, .sleep_us = remain};

    esp_now_send(info->src_addr, (uint8_t*)&reply, sizeof(reply));*/
}

void init_espnow(void) {
  ESP_LOGI(TAG, "Initialisation ESP-NOW");
  ESP_ERROR_CHECK(esp_now_init());
  ESP_ERROR_CHECK(esp_now_register_send_cb(on_data_sent));
  ESP_ERROR_CHECK(esp_now_register_recv_cb(on_data_recv));

  /* broadcast peer */
  esp_now_peer_info_t bcast = {
      .peer_addr = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
      .channel = NORMAL_CHANNEL,
      .ifidx = WIFI_IF_STA,
      .encrypt = false};
  ESP_ERROR_CHECK(esp_now_add_peer(&bcast));
}

void send_adv() {
  ESP_LOGI(TAG, "Début de synchronisation");
  sync_msg_t sync = {.type = MSG_SYNC, .cycle_us = CYCLE_US};
  esp_err_t err = esp_now_send(broadcast_mac, (uint8_t*)&sync, sizeof(sync));
  ESP_LOGI(TAG, "esp_now_send returned: %s", esp_err_to_name(err));
}