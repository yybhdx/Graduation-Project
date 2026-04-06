#ifndef WIFI_MQTT_H
#define WIFI_MQTT_H

#include <stdbool.h>

void wifi_mqtt_init(void);
void wifi_mqtt_start(void);
bool wifi_mqtt_is_connected(void);
int wifi_mqtt_publish(const char *topic, const char *data);

#endif
