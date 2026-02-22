#include "espnow.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "gpioconfig.h"
#include "loramanager.h"
#include "nvs_flash.h"
#include "sensors.h"
#include "sigfoxmanager.h"

static const char* TAG = "ESPNOW";
#define NVS_NAMESPACE "espnow"
uint8_t broadcast_mac[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
uint8_t dip_switch_value;
float temperature;
float humidity;
float voltage;

void init_wifi(void) {
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "Initialisation WiFi...");
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_ERROR_CHECK(esp_wifi_set_channel(NORMAL_CHANNEL, WIFI_SECOND_CHAN_NONE));
  ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
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
  if (len <= 0 || data == NULL) {
    ESP_LOGW(TAG, "Message vide ou invalide");
    return;
  }

  ESP_LOGI(TAG, "Message reçu type=%d, len=%d", data[0], len);
  ESP_LOGI(TAG, "Message reçu de %02X:%02X:%02X:%02X:%02X:%02X, len=%d",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5], len);
  if (data[0] != MSG_DATA) return;
  if (len != sizeof(client_msg_t)) {
    ESP_LOGW(TAG, "Taille inattendue: %d (attendu %d)", len,
             sizeof(client_msg_t));
    return;
  }
  client_msg_t msg;
  memcpy(&msg, data, sizeof(client_msg_t));

  // Mise à jour des champs station
  msg.station_temp_x10 = (int16_t)(temperature * 10.0f);
  msg.station_humidity_x100 = (uint16_t)humidity;
  msg.station_battery_mv = (uint16_t)voltage;

  ESP_LOGI(TAG,
           "UID %d | %.2f kg | %.2f °C | %.3f V | "
           "Station: %.2f °C | %d %% | %.3f V",
           msg.uid, msg.weight_x10 / 10.0f, msg.temp_x10 / 10.0f,
           msg.battery_mv / 1000.0f, msg.station_temp_x10 / 10.0f,
           msg.station_humidity_x100, msg.station_battery_mv / 1000.0f);

  sendData(&msg);
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
  esp_err_t err = esp_now_add_peer(&bcast);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "Peer broadcast enregistré");
  } else if (err == ESP_ERR_ESPNOW_EXIST) {
    ESP_LOGI(TAG, "Peer broadcast existait déjà");
  } else {
    ESP_LOGE(TAG, "Erreur enregistrement peer broadcast: %s",
             esp_err_to_name(err));
  }
}

void send_adv(uint8_t _dip_switch_value, float _temperature, float _humidity,
              float _voltage) {
  ESP_LOGI(TAG, "Début de synchronisation");
  dip_switch_value = _dip_switch_value;

  temperature = _temperature;
  ESP_LOGI(TAG, "temperature: %.2f, _temperature: %.2f", temperature,
           _temperature);
  humidity = _humidity;
  ESP_LOGI(TAG, "humidity: %.2f, _humidity: %.2f", humidity, _humidity);
  voltage = _voltage;
  ESP_LOGI(TAG, "voltage: %.2f, _voltage: %.2f", voltage, _voltage);
  sync_msg_t sync = {.type = MSG_SYNC, .cycle_us = CYCLE_US};
  esp_err_t err = esp_now_send(broadcast_mac, (uint8_t*)&sync, sizeof(sync));
  ESP_LOGI(TAG, "esp_now_send returned: %s", esp_err_to_name(err));
}

void sendData(const client_msg_t* data) {
  ESP_LOGI(TAG, "📥 Données reçues via ESP-NOW :");
  ESP_LOGI(TAG, "--------------------------------");
  ESP_LOGI(TAG, "🆔 UID : %d", data->uid);
  ESP_LOGI(TAG, "🌡️ Température : %.2f °C", data->temp_x10 / 10.0f);
  ESP_LOGI(TAG, "⚖️ Poids : %.2f kg", data->weight_x10 / 10.0f);
  ESP_LOGI(TAG, "⚡ Voltage : %.2f V", data->battery_mv / 1000.0f);
  ESP_LOGI(TAG, "🗼 Station");
  ESP_LOGI(TAG, "🌡️ Température : %.2f °C", data->station_temp_x10 / 10.0f);
  ESP_LOGI(TAG, "💧 Humidité : %d %%", data->station_humidity_x100);
  ESP_LOGI(TAG, "⚡ Voltage : %.2f V", data->station_battery_mv / 1000.0f);
  ESP_LOGI(TAG, "--------------------------------");

  // Encodage des données dans un payload
  uint16_t poidsEncoded = (uint16_t)(data->weight_x10 * 10);
  int16_t temperatureEncoded = (int16_t)(data->temp_x10 * 10);
  uint8_t voltageEncoded = (data->battery_mv * 100) / 2;

  int16_t temperatureStationEncoded = (int16_t)(data->station_temp_x10 * 10);
  uint8_t humidityStationEncoded = data->station_humidity_x100;
  uint8_t voltageStationEncoded = (data->station_battery_mv * 100) / 2;

  uint8_t payload[10];
  payload[0] = data->uid;
  payload[1] = (poidsEncoded >> 8) & 0xFF;
  payload[2] = poidsEncoded & 0xFF;
  payload[3] = (temperatureEncoded >> 8) & 0xFF;
  payload[4] = temperatureEncoded & 0xFF;
  payload[5] = voltageEncoded;
  payload[6] = (temperatureStationEncoded >> 8) & 0xFF;
  payload[7] = temperatureStationEncoded & 0xFF;
  payload[8] = humidityStationEncoded;
  payload[9] = voltageStationEncoded;

  char message[24];
  snprintf(message, sizeof(message), "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
           payload[0], payload[1], payload[2], payload[3], payload[4],
           payload[5], payload[6], payload[7], payload[8], payload[9]);

  if (is_lora_enabled()) {
    ESP_LOGI(TAG, "📡 Envoie via Lora");
    sendDataViaLora(message);
  } else if (is_sigfox_enabled()) {
    ESP_LOGI(TAG, "📡 Envoie via Sigfox");
    sendDataViaSigfox(message);
  } else {
    ESP_LOGI(TAG, "⚠️ Aucun réseau activé, données non envoyées");
  }
}