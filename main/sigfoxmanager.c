#include "sigfoxmanager.h"

#include <stdio.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpioconfig.h"

#define TAG "SIGFOX"

#define BUF_SIZE 256

void initSigfox(void) {
  esp_log_level_set(TAG, ESP_LOG_INFO);
  // UART config
  uart_config_t uart_config = {
      .baud_rate = SIGFOX_BAUD,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
  };

  uart_driver_install(SIGFOX_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
  uart_param_config(SIGFOX_UART_NUM, &uart_config);
  uart_set_pin(SIGFOX_UART_NUM, SIGFOX_TX_PIN, SIGFOX_RX_PIN,
               UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  ESP_LOGI(TAG, "Sigfox UART initialized");
}

void preparePayload(const char* input, char* output, size_t max_len) {
  strncpy(output, input, max_len);
  output[max_len - 1] = '\0';
}

bool sendDataViaSigfox(const char* data) {
  char payload[25];
  preparePayload(data, payload, sizeof(payload));

  char command[64];
  snprintf(command, sizeof(command), "AT$SF=%s,0,1\r\n", payload);

  const int maxRetries = 3;
  uint8_t rx_buffer[BUF_SIZE];

  for (int attempt = 1; attempt <= maxRetries; attempt++) {
    ESP_LOGI(TAG, "Tentative #%d", attempt);

    uart_flush(SIGFOX_UART_NUM);
    uart_write_bytes(SIGFOX_UART_NUM, command, strlen(command));

    int64_t start = esp_timer_get_time();
    char response[128] = {0};
    int total_len = 0;

    while ((esp_timer_get_time() - start) < 5000000) {  // 5s

      int len = uart_read_bytes(SIGFOX_UART_NUM, rx_buffer,
                                sizeof(rx_buffer) - 1, pdMS_TO_TICKS(100));

      if (len > 0) {
        rx_buffer[len] = 0;
        strcat(response, (char*)rx_buffer);
        total_len += len;

        if (strstr(response, "OK")) {
          ESP_LOGI(TAG, "Message envoyé avec succès");
          return true;
        }
      }
    }

    ESP_LOGW(TAG, "Timeout, nouvelle tentative...");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  ESP_LOGE(TAG, "Echec complet Sigfox");
  return false;
}

void sendSigfoxATCommand(const char* command) {
  uint8_t rx_buffer[256];
  char response[256] = {0};

  ESP_LOGI("SIGFOX", "Envoi commande: %s", command);

  // Nettoyer le buffer UART
  uart_flush(SIGFOX_UART_NUM);

  // Envoyer la commande
  uart_write_bytes(SIGFOX_UART_NUM, command, strlen(command));

  int64_t start = esp_timer_get_time();
  int total_len = 0;

  while ((esp_timer_get_time() - start) < 5000000) {  // 5 secondes timeout

    int len = uart_read_bytes(SIGFOX_UART_NUM, rx_buffer, sizeof(rx_buffer) - 1,
                              pdMS_TO_TICKS(200));

    if (len > 0) {
      rx_buffer[len] = 0;

      if (total_len + len < sizeof(response) - 1) {
        strcat(response, (char*)rx_buffer);
        total_len += len;
      }
    }
  }

  if (total_len > 0) {
    ESP_LOGI("SIGFOX", "Réponse:\n%s", response);
  } else {
    ESP_LOGW("SIGFOX", "Aucune réponse (timeout)");
  }
}