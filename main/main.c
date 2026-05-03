#include <string.h>

#include "esp_event.h"
#include "esp_log.h"
#include "esp_now.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "espnow.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "loramanager.h"
#include "nvs_flash.h"
#include "sensors.h"
#include "sigfoxmanager.h"

static const char* TAG = "ESPNOW_SERVER";
TimerHandle_t sleep_timer;
TaskHandle_t main_task_handle = NULL;
void timer_callback(TimerHandle_t xTimer) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  vTaskNotifyGiveFromISR(main_task_handle, &xHigherPriorityTaskWoken);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void app_main(void) {
  main_task_handle = xTaskGetCurrentTaskHandle();
  esp_log_level_set("*", ESP_LOG_WARN);
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "🚀 Démarrage serveur ESP-NOW...");
  init_sensors();
  if (is_sigfox_enabled()) {
    ESP_LOGI(TAG, "📡 Initialisation du module Sigfox...");
    initSigfox();
  } else if (is_lora_enabled()) {
    ESP_LOGI(TAG, "📡 Initialisation du module LoRa...");
    initLora();
  }
  vTaskDelay(pdMS_TO_TICKS(100));
  init_wifi();
  vTaskDelay(pdMS_TO_TICKS(100));
  init_espnow();
  vTaskDelay(pdMS_TO_TICKS(500));
  ESP_LOGI(TAG, "✅ Serveur prêt à recevoir les messages !");

  uint8_t dip_switch_value = read_address_dip_switch();
  float temperature = read_temperature();
  float humidity = read_humidity();
  float voltage = read_voltage();

  send_adv(dip_switch_value, temperature, humidity, voltage);

  uint8_t NB_SLOTS = read_nodes_dip_switch();

  TimerHandle_t sleep_timer =
      xTimerCreate("sleep_timer", pdMS_TO_TICKS(NB_SLOTS * SLOT_MS + 2000),
                   pdFALSE, NULL, timer_callback);

  xTimerStart(sleep_timer, 0);

  // Attente notification
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  ESP_LOGI(TAG, "Mise en veille");

  esp_now_deinit();
  esp_wifi_stop();

  ESP_LOGI(TAG, "Dodo pour %lld minutes", CYCLE_US / 1000000LL / 60);
  esp_sleep_enable_timer_wakeup(CYCLE_US);
  esp_deep_sleep_start();
}
