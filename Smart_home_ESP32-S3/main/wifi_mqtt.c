#include "wifi_mqtt.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "mqtt_client.h"

#define WIFI_SSID "jf"
#define WIFI_PASS "a8pswys108"
#define HUAWEI_MQTT_URL "mqtt://52e4e17470.st1.iotda-device.cn-east-3.myhuaweicloud.com"
#define HUAWEI_CLIENT_ID "69ce6bd8e094d615922d9e08_Smart_Home_0_0_2026040213"
#define HUAWEI_USERNAME  "69ce6bd8e094d615922d9e08_Smart_Home"
#define HUAWEI_PASSWORD  "b859e0be5c2f5ed05ec764914e485d1204b37bb341afe10c91d4a9c8dae43a19"

static const char *TAG = "WIFI_MQTT";
static esp_mqtt_client_handle_t client = NULL;
static bool connected = false;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED: connected = true; ESP_LOGI(TAG, "MQTT Connected"); break;
        case MQTT_EVENT_DISCONNECTED: connected = false; ESP_LOGW(TAG, "MQTT Disconnected"); break;
        default: break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data) {
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) esp_wifi_connect();
    else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) ESP_LOGI(TAG, "Got IP");
}

void wifi_mqtt_init(void) {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    wifi_config_t wifi_config = {.sta = {.ssid = WIFI_SSID, .password = WIFI_PASS}};
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

void wifi_mqtt_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = HUAWEI_MQTT_URL,
        .broker.address.port = 1883,
        .credentials.client_id = HUAWEI_CLIENT_ID,
        .credentials.username = HUAWEI_USERNAME,
        .credentials.authentication.password = HUAWEI_PASSWORD,
    };
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

bool wifi_mqtt_is_connected(void) { return connected; }

int wifi_mqtt_publish(const char *topic, const char *data) {
    if (connected && client) return esp_mqtt_client_publish(client, topic, data, 0, 1, 0);
    return -1;
}
