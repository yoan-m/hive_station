#include "sensors.h"

#include "bme280.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "gpioconfig.h"
#include "sdkconfig.h"

static const char* TAG = "SENSORS";

uint8_t UID = 0x00;

/* =========================
 * ADC
 * ========================= */

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;

static i2c_bus_handle_t i2c_bus = NULL;
static bme280_handle_t bme280 = NULL;

/* =========================
 * INIT
 * ========================= */

static void blink(void) {
  for (int i = 0; i < 2; i++) {
    gpio_set_level(LED_BUILTIN, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_BUILTIN, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_BUILTIN, 1);
    vTaskDelay(pdMS_TO_TICKS(200));
    gpio_set_level(LED_BUILTIN, 0);
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void init_i2c(void) {
  ESP_LOGI(TAG, "Initialisation bus I2C");
  i2c_config_t conf = {
      .mode = I2C_MODE_MASTER,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .sda_pullup_en = GPIO_PULLUP_ENABLE,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .scl_pullup_en = GPIO_PULLUP_ENABLE,
      .master.clk_speed = I2C_MASTER_FREQ_HZ,
  };
  i2c_bus = i2c_bus_create(I2C_MASTER_NUM, &conf);
}

void init_bme280(void) {
  ESP_LOGI(TAG, "Initialisation capteur BME280");
  bme280 = bme280_create(i2c_bus, BME280_I2C_ADDRESS_DEFAULT);
  bme280_default_init(bme280);
}

void init_lora_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin Lora");
  gpio_config_t in_pullup = {.mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};

  in_pullup.pin_bit_mask = 1ULL << LORA_PIN;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "Lora pin initialized");
}

void init_sigfox_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin Sigfox");
  gpio_config_t in_pullup = {.mode = GPIO_MODE_INPUT,
                             .pull_up_en = GPIO_PULLUP_DISABLE,
                             .pull_down_en = GPIO_PULLDOWN_DISABLE,
                             .intr_type = GPIO_INTR_DISABLE};

  in_pullup.pin_bit_mask = 1ULL << SIGFOX_PIN;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "Sigfox pin initialized");
}

void init_led_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin LED");
  gpio_config_t led_cfg = {
      .pin_bit_mask = 1ULL << LED_BUILTIN,
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&led_cfg);
  gpio_set_level(LED_BUILTIN, 0);
  blink();
  ESP_LOGI(TAG, "LED pin initialized");
}

void init_setup_pin(void) {
  ESP_LOGI(TAG, "Initialisation pin setup");
  gpio_config_t in_pullup = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  in_pullup.pin_bit_mask = 1ULL << SETUP_PIN;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "Setup pin initialized");
}

void init_adc(void) {
  ESP_LOGI(TAG, "Initialisation ADC");
  gpio_config_t adc_power = {
      .pin_bit_mask = 1ULL << POWER_ADC_PIN,
      .mode = GPIO_MODE_OUTPUT,
  };
  gpio_config(&adc_power);
  gpio_set_level(POWER_ADC_PIN, 0);

  // -------- ADC init --------
  adc_oneshot_unit_init_cfg_t unit_cfg = {
      .unit_id = ADC_UNIT_ID,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

  adc_oneshot_chan_cfg_t chan_cfg = {
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .atten = ADC_ATTEN,
  };
  ESP_ERROR_CHECK(
      adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &chan_cfg));

  // -------- Calibration --------
  adc_cali_line_fitting_config_t cali_cfg = {
      .atten = ADC_ATTEN,
      .bitwidth = ADC_BITWIDTH_DEFAULT,
      .unit_id = ADC_UNIT_ID,
  };

  ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle));

  ESP_LOGI(TAG, "ADC prêt");
}

void init_dip_switches(void) {
  ESP_LOGI(TAG, "Initialisation DIP switches");
  gpio_config_t in_pullup = {
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
  };

  uint64_t dip_mask = 0;
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    dip_mask |= 1ULL << DIP_SWITCH_PINS[i];
  }
  in_pullup.pin_bit_mask = dip_mask;
  gpio_config(&in_pullup);
  ESP_LOGI(TAG, "DIP switches initialized");
}

void init_sensors(void) {
  esp_log_level_set(TAG, ESP_LOG_INFO);
  ESP_LOGI(TAG, "Initialisation des capteurs...");
  init_setup_pin();
  init_led_pin();

  init_adc();

  init_lora_pin();
  init_sigfox_pin();

  init_dip_switches();
  read_dip_switch();

  init_i2c();
  init_bme280();

  ESP_LOGI(TAG, "Sensors initialized");
}

/* =========================
 * READ FUNCTIONS
 * ========================= */

float read_temperature(void) {
  if (!bme280) {
    ESP_LOGE(TAG, "BME280 non initialisé");
    return 0.0;
  }
  float temperature;

  bme280_read_temperature(bme280, &temperature);

  ESP_LOGI(TAG, "Temp: %.2f °C\n", temperature);
  return temperature;
}

float read_humidity(void) {
  if (!bme280) {
    ESP_LOGE(TAG, "BME280 non initialisé");
    return 0.0;
  }

  float humidity;

  bme280_read_humidity(bme280, &humidity);

  ESP_LOGI(TAG, "Humidity: %.2f %%", humidity);
  return humidity;
}

float read_pressure(void) {
  if (!bme280) {
    ESP_LOGE(TAG, "BME280 non initialisé");
    return 0.0;
  }

  float pressure;

  bme280_read_pressure(bme280, &pressure);

  ESP_LOGI(TAG, "Pressure: %.2f hPa", pressure / 100.0);
  return pressure;
}

bool is_setup_enabled(void) {
  if (gpio_get_level(SETUP_PIN) == 0) {
    ESP_LOGI(TAG, "Setup mode enabled");
    return true;
  }
  return false;
}

bool is_sigfox_enabled(void) {
  if (gpio_get_level(SIGFOX_PIN) == 1) {
    ESP_LOGI(TAG, "Sigfox enabled");
    return true;
  }
  return false;
}

bool is_lora_enabled(void) {
  if (gpio_get_level(LORA_PIN) == 1) {
    ESP_LOGI(TAG, "Lora enabled");
    return true;
  }
  return false;
}

float read_voltage(void) {
  gpio_set_level(POWER_ADC_PIN, 1);
  vTaskDelay(pdMS_TO_TICKS(200));

  int raw;
  int mv;

  ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &raw));
  ESP_ERROR_CHECK(adc_cali_raw_to_voltage(cali_handle, raw, &mv));

  int voltage = raw * (3.3 / 4095.0) * 1000;  // Convertir en tension ADC
  int batteryVoltage =
      voltage / (100.0 / (100.0 + 100.0));  // Ajustement avec le pont diviseur
  ESP_LOGI(TAG, "RAW=%d  Voltage=%d mV RealVoltage=%d mV batteryVoltage=%d mV",
           raw, mv, voltage, batteryVoltage);

  gpio_set_level(POWER_ADC_PIN, 0);
  return mv;
}

uint8_t read_dip_switch(void) {
  uint8_t id = 0;
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    if (gpio_get_level(DIP_SWITCH_PINS[i]) == 0) {
      id |= (1 << i);
    }
  }
  UID = id + 1;

  ESP_LOGI(TAG, "Valeur du DIP switch : %d", UID);
  return UID;
}

void disable_input(gpio_num_t gpio) { gpio_reset_pin(gpio); }

void disable_inputs(void) {
  for (int i = 0; i < DIP_SWITCH_PINS_COUNT; i++) {
    disable_input(DIP_SWITCH_PINS[i]);
    gpio_reset_pin(DIP_SWITCH_PINS[i]);
    gpio_set_direction(DIP_SWITCH_PINS[i], GPIO_MODE_INPUT);
    gpio_set_pull_mode(DIP_SWITCH_PINS[i], GPIO_FLOATING);
  }

  // gpio_set_level(POWER_SENSORS_PIN, 0);  // coupe capteurs

  disable_input(LORA_PIN);
  gpio_reset_pin(LORA_PIN);
  gpio_set_direction(LORA_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(LORA_PIN, GPIO_FLOATING);

  disable_input(SIGFOX_PIN);
  gpio_reset_pin(SIGFOX_PIN);
  gpio_set_direction(SIGFOX_PIN, GPIO_MODE_INPUT);
  gpio_set_pull_mode(SIGFOX_PIN, GPIO_FLOATING);

  gpio_reset_pin(I2C_MASTER_SCL_IO);
  gpio_reset_pin(I2C_MASTER_SDA_IO);
  gpio_set_direction(I2C_MASTER_SCL_IO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(I2C_MASTER_SCL_IO, GPIO_FLOATING);
  gpio_set_direction(I2C_MASTER_SDA_IO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(I2C_MASTER_SDA_IO, GPIO_FLOATING);
}

uint8_t get_UID(void) { return UID; }
