
#ifndef _MQTT_CLIENT_TRANSPORT_H
#define _MQTT_CLIENT_TRANSPORT_H
#include "mqtt_client.h"
#include "stdint.h"
#include "stdio.h"
#include "string.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int mqttClientSendData(MqttClientContext *context,
                           const char *data, uint16_t length, uint16_t *written, uint8_t flags);

    int mqttClientReceiveData(MqttClientContext *context,
                              char *data, uint16_t size, uint16_t *received, uint8_t flags);

    int mqttClientWaitForData(MqttClientContext *context, uint64_t timeout);

#ifdef __cplusplus
}
#endif

#endif