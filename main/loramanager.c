#include "loramanager.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "espnow.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpioconfig.h"
#include "sensors.h"

static const char* TAG = "LORA";

// === Initialisation du LoRa ===
void initLora() {
  esp_log_level_set(TAG, ESP_LOG_INFO);
  // Configurer l'UART
  const uart_config_t uart_config = {.baud_rate = 115200,
                                     .data_bits = UART_DATA_8_BITS,
                                     .parity = UART_PARITY_DISABLE,
                                     .stop_bits = UART_STOP_BITS_1,
                                     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE};
  uart_driver_install(UART_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(UART_NUM, &uart_config);
  uart_set_pin(UART_NUM, LORA_TX_PIN, LORA_RX_PIN, UART_PIN_NO_CHANGE,
               UART_PIN_NO_CHANGE);

  vTaskDelay(pdMS_TO_TICKS(1000));
  if (is_setup_enabled()) {
    ESP_LOGI(TAG, "Mode setup activé, configuration LoRa en cours...");
    sendLoraATCommand("AT+ADDRESS=1", WAIT_MS);
    sendLoraATCommand("AT+NETWORKID=5", WAIT_MS);
    sendLoraATCommand("AT+BAND=868000000,M", WAIT_MS);
    sendLoraATCommand("AT+PARAMETER=9,7,1,12", WAIT_MS);
    ESP_LOGI(TAG, "Configuration LoRa terminée.");
  } else {
    ESP_LOGI(TAG, "Mode setup désactivé, configuration LoRa ignorée.");
  }

  ESP_LOGI(TAG, "Émetteur prêt.");
}

// === Préparer le payload LoRa (max 24 caractères ASCII) ===
void preparePayloadLora(const char* input, char* output, size_t size) {
  ESP_LOGI(TAG, "Preparation du payload : %s", input);
  size_t len = strlen(input);
  if (len > 24) len = 24;
  strncpy(output, input, len);
  output[len] = '\0';
}

// === Envoyer une commande AT et lire la réponse ===
void sendLoraATCommand(const char* cmd, uint32_t wait_ms) {
  ESP_LOGI(TAG, "📨 Envoi : %s", cmd);

  uart_flush(UART_NUM);

  uart_write_bytes(UART_NUM, cmd, strlen(cmd));
  uart_write_bytes(UART_NUM, "\r\n", 2);

  uint8_t data[UART_BUF_SIZE];
  int total_len = 0;

  int64_t start = esp_timer_get_time();

  while ((esp_timer_get_time() - start) < (wait_ms * 1000)) {
    int len = uart_read_bytes(UART_NUM, data + total_len,
                              UART_BUF_SIZE - 1 - total_len, pdMS_TO_TICKS(50));

    if (len > 0) {
      total_len += len;
      data[total_len] = '\0';

      // DEBUG
      ESP_LOGI(TAG, "📥 Reçu: %s", data);

      // ✔️ Vérification des réponses
      if (strstr((char*)data, "+OK")) {
        return;
      }
      if (strstr((char*)data, "+ERR")) {
        return;
      }
      if (strstr((char*)data, "+SENT")) {
        return;
      }
    }
  }

  ESP_LOGW(TAG, "⏱ Timeout sans réponse");
  return;
}

// === Envoyer les données via LoRa ===
void sendDataViaLora(const char* data) {
  ESP_LOGI(TAG, "📨 Envoi via LoRa");
  char payload[32];
  preparePayloadLora(data, payload, sizeof(payload));

  char command[64];
  snprintf(command, sizeof(command), "AT+SEND=2,%d,%s", (int)strlen(payload),
           payload);

  sendLoraATCommand("AT+MODE=0", WAIT_MS);
  sendLoraATCommand(command, WAIT_MS);
  sendLoraATCommand("AT+MODE=1", WAIT_MS);
}
