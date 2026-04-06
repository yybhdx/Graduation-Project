#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "audio_manager.h"
#include "wifi_mqtt.h"

#define STM32_UART_PORT UART_NUM_2
#define STM32_TX_PIN (4)
#define STM32_RX_PIN (5)
#define UART_BUF_SIZE (1024)
#define HUAWEI_PUB_TOPIC "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

static const char *TAG = "MAIN_APP";

void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(STM32_UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(STM32_UART_PORT, &uart_config);
    uart_set_pin(STM32_UART_PORT, STM32_TX_PIN, STM32_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void stm32_task(void *arg) {
    uint8_t *data = malloc(UART_BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(STM32_UART_PORT, data, UART_BUF_SIZE - 1, pdMS_TO_TICKS(50));
        if (len > 0) {
            data[len] = '\0';
            ESP_LOGI(TAG, "From STM32: %s", (char *)data);
            if (wifi_mqtt_is_connected() && data[0] == '{') {
                wifi_mqtt_publish(HUAWEI_PUB_TOPIC, (char *)data);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    // 1. NVS 初始化
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 2. 模块初始化
    audio_manager_init();
    uart_init();
    wifi_mqtt_init();

    // 3. 启动任务
    audio_manager_start_loopback();
    xTaskCreate(stm32_task, "stm32_task", 4096, NULL, 10, NULL);

    // 4. 等待网络稳定后启动 MQTT
    vTaskDelay(pdMS_TO_TICKS(5000));
    wifi_mqtt_start();
}
