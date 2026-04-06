#include "audio_manager.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "AUDIO_MANAGER";
static i2s_chan_handle_t tx_handle = NULL;
static i2s_chan_handle_t rx_handle = NULL;

void audio_manager_init(void) {
    // TX 配置 (Speaker)
    i2s_chan_config_t tx_chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    i2s_new_channel(&tx_chan_cfg, &tx_handle, NULL);
    i2s_std_config_t tx_std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED, .bclk = SPK_I2S_BCLK, .ws = SPK_I2S_LRC, .dout = SPK_I2S_DIN, .din = I2S_GPIO_UNUSED}
    };
    i2s_channel_init_std_mode(tx_handle, &tx_std_cfg);

    // RX 配置 (Mic)
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
    ESP_LOGI(TAG, "I2S initialized (16kHz, Mono)");
}

static void audio_loopback_task(void *args) {
    const int SAMPLE_CNT = 256;
    size_t bytes_read = 0, bytes_written = 0;
    int32_t *read_buf = malloc(SAMPLE_CNT * sizeof(int32_t));
    int16_t *write_buf = malloc(SAMPLE_CNT * sizeof(int16_t));

    while (1) {
        if (i2s_channel_read(rx_handle, read_buf, SAMPLE_CNT * sizeof(int32_t), &bytes_read, portMAX_DELAY) == ESP_OK) {
            int samples = bytes_read / sizeof(int32_t);
            for (int i = 0; i < samples; i++) {
                write_buf[i] = (int16_t)(read_buf[i] >> 16);
            }
            i2s_channel_write(tx_handle, write_buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        }
    }
}

void audio_manager_start_loopback(void) {
    xTaskCreate(audio_loopback_task, "audio_task", 4096, NULL, 15, NULL);
}
