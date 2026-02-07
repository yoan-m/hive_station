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
#include "nvs_flash.h"

static const char* TAG = "ESPNOW_SERVER";
void app_main(void) {
  nvs_flash_init();
  init_wifi();
  init_espnow();

  ESP_LOGI(TAG, "Serveur démarré\n");

  send_adv();
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  /*vTaskDelay(pdMS_TO_TICKS(NB_SLOTS * SLOT_MS + 200));

  ESP_LOGI(TAG, "Mise en veille\n");
  esp_sleep_enable_timer_wakeup(CYCLE_US);
  esp_deep_sleep_start();*/
}
