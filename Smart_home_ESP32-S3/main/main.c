#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/uart.h"
#include "mqtt_client.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"

// =================== 配置参数 ===================
#define WIFI_SSID "jf"
#define WIFI_PASS "a8pswys108"

#define HUAWEI_MQTT_URL "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_MQTT_PORT 1883
#define HUAWEI_CLIENT_ID "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213"
#define HUAWEI_USERNAME "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"
#define HUAWEI_PUB_TOPIC "$oc/devices/69ce6bd8e094d615922d9e08_Smart_Home/sys/properties/report"

// --- 更改 UART 引脚，避开 PSRAM 冲突 ---
#define STM32_UART_PORT UART_NUM_2
#define STM32_TX_PIN (4) // 建议改到 GPIO 4
#define STM32_RX_PIN (5) // 建议改到 GPIO 5
#define UART_BUF_SIZE (1024)

// =================== 音频 I2S 引脚配置 ===================
#define MIC_I2S_SCK (12)
#define MIC_I2S_WS (11)
#define MIC_I2S_SD (10)

#define SPK_I2S_BCLK (16)
#define SPK_I2S_LRC (15)
#define SPK_I2S_DIN (7)

static const char *TAG = "SMART_HOME_S3";
esp_mqtt_client_handle_t mqtt_client = NULL;
bool is_mqtt_connected = false;

i2s_chan_handle_t tx_handle = NULL;
i2s_chan_handle_t rx_handle = NULL;

/**
 * @brief 初始化音频 I2S 通道
 */
void i2s_driver_init(void)
{
    // --- TX 配置 (MAX98357) ---
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&tx_chan_cfg, &tx_handle, NULL);
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000), 
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = SPK_I2S_BCLK, .ws = SPK_I2S_LRC, .dout = SPK_I2S_DIN, .din = I2S_GPIO_UNUSED}
    };
    i2s_channel_init_std_mode(tx_handle, &tx_std_cfg);

    // --- RX 配置 (INMP441) ---
    i2s_chan_config_t rx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);
    i2s_new_channel(&rx_chan_cfg, NULL, &rx_handle);
    i2s_std_config_t rx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = MIC_I2S_SCK, .ws = MIC_I2S_WS, .dout = I2S_GPIO_UNUSED, .din = MIC_I2S_SD}
    };
    i2s_channel_init_std_mode(rx_handle, &rx_std_cfg);

    i2s_channel_enable(tx_handle);
    i2s_channel_enable(rx_handle);
}

/**
 * @brief 音频回环任务
 */
void audio_loopback_task(void *args)
{
    const int SAMPLE_CNT = 256; // 减小缓冲区减少延迟和回音
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    
    int32_t *read_buf = (int32_t *)malloc(SAMPLE_CNT * sizeof(int32_t));
    int16_t *write_buf = (int16_t *)malloc(SAMPLE_CNT * sizeof(int16_t));

    while (1)
    {
        // 阻塞读取
        if (i2s_channel_read(rx_handle, read_buf, SAMPLE_CNT * sizeof(int32_t), &bytes_read, portMAX_DELAY) == ESP_OK)
        {
            int samples = bytes_read / sizeof(int32_t);
            for (int i = 0; i < samples; i++)
            {
                // INMP441 的数据在 32 位槽位中的高 24 位
                // 这里的处理非常关键：
                int32_t temp = read_buf[i];
                
                // 1. 如果你发现完全没说话声只有噪音，可能是符号位错了，尝试：
                // temp = temp << 0; // 不处理

                // 2. 映射到 16 位：右移 14 位（保留一点动态余量防止爆音）
                write_buf[i] = (int16_t)(temp >> 18);
            }
            // 写入输出
            i2s_channel_write(tx_handle, write_buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        }
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Huawei Cloud MQTT Connected!");
        is_mqtt_connected = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "Huawei Cloud MQTT Disconnected!");
        is_mqtt_connected = false;
        break;
    default:
        break;
    }
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
        esp_wifi_connect();
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(void)
{
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL, NULL);

    wifi_config_t wifi_config = {.sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}};
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void mqtt_app_start(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = HUAWEI_MQTT_PORT,
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void stm32_rx_task(void *arg)
{
    uint8_t *data = (uint8_t *)malloc(UART_BUF_SIZE);
    while (1)
    {
        int len = uart_read_bytes(STM32_UART_PORT, data, UART_BUF_SIZE - 1, 50 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            data[len] = '\0';
            char *payload = (char *)data;
            // --- 必须把打印加回来，否则你根本不知道收没收到 ---
            ESP_LOGI(TAG, "Recv from STM32: %s", payload);

            if (is_mqtt_connected && payload[0] == '{')
            {
                int msg_id = esp_mqtt_client_publish(mqtt_client, HUAWEI_PUB_TOPIC, payload, 0, 1, 0);
                ESP_LOGI(TAG, "MQTT Sent OK, ID=%d", msg_id);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void uart_init(void)
{
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

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    i2s_driver_init();
    uart_init();
    wifi_init_sta();

    // 先创建任务
    xTaskCreate(stm32_rx_task, "stm32_rx_task", 4096, NULL, 10, NULL);
    xTaskCreate(audio_loopback_task, "audio_loopback_task", 4096, NULL, 15, NULL);

    vTaskDelay(pdMS_TO_TICKS(5000));
    mqtt_app_start();
}