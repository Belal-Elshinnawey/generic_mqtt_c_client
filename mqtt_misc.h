#ifndef _MQTT_CLIENT_MISC_H
#define _MQTT_CLIENT_MISC_H

#include "mqtt_client.h"
#include "stdint.h"
#include "stdbool.h"
#include "string.h"
#include <inttypes.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void mqttClientChangeState(MqttClientContext *context,
                               MqttClientState newState);

    int mqttClientProcessEvents(MqttClientContext *context, uint64_t timeout);
    int mqttClientCheckKeepAlive(MqttClientContext *context);

    int mqttSerializeHeader(uint8_t *buffer, uint16_t *pos, MqttPacketType type,
                            bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttSerializeByte(uint8_t *buffer, uint16_t bufferLen,
                          uint16_t *pos, uint8_t value);

    int mqttSerializeShort(uint8_t *buffer, uint16_t bufferLen,
                           uint16_t *pos, uint16_t value);

    int mqttSerializeString(uint8_t *buffer, uint16_t bufferLen,
                            uint16_t *pos, const void *string, uint16_t stringLen);

    int mqttSerializeData(uint8_t *buffer, uint16_t bufferLen,
                          uint16_t *pos, const void *data, uint16_t dataLen);

    int mqttDeserializeHeader(uint8_t *buffer, uint16_t bufferLen, uint16_t *pos,
                              MqttPacketType *type, bool *dup, MqttQosLevel *qos, bool *retain, uint16_t *remainingLen);

    int mqttDeserializeByte(uint8_t *buffer, uint16_t bufferLen,
                            uint16_t *pos, uint8_t *value);

    int mqttDeserializeShort(uint8_t *buffer, uint16_t bufferLen,
                             uint16_t *pos, uint16_t *value);

    int mqttDeserializeString(uint8_t *buffer, uint16_t bufferLen,
                              uint16_t *pos, char **string, uint16_t *stringLen);

    int mqttClientCheckTimeout(MqttClientContext *context);

#ifdef __cplusplus
}
#endif

#endif