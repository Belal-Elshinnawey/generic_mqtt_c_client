#ifndef _MQTT_CLIENT_PACKET_H
#define _MQTT_CLIENT_PACKET_H

#include "mqtt_client.h"
#include "stdio.h"
#include "stdint.h"
#include "string.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int mqttClientReceivePacket(MqttClientContext *context);
    int mqttClientProcessPacket(MqttClientContext *context);

    int mqttClientProcessConnAck(MqttClientContext *context,
                                 bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPubAck(MqttClientContext *context,
                                bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPublish(MqttClientContext *context,
                                 bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPubRec(MqttClientContext *context,
                                bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPubRel(MqttClientContext *context,
                                bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPubComp(MqttClientContext *context,
                                 bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessSubAck(MqttClientContext *context,
                                bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessUnsubAck(MqttClientContext *context,
                                  bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientProcessPingResp(MqttClientContext *context,
                                  bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen);

    int mqttClientFormatConnect(MqttClientContext *context,
                                bool cleanSession);

    int mqttClientFormatPublish(MqttClientContext *context, const char *topic,
                                const void *message, uint16_t length, MqttQosLevel qos, bool retain);

    int mqttClientFormatPubAck(MqttClientContext *context, uint16_t packetId);
    int mqttClientFormatPubRec(MqttClientContext *context, uint16_t packetId);
    int mqttClientFormatPubRel(MqttClientContext *context, uint16_t packetId);
    int mqttClientFormatPubComp(MqttClientContext *context, uint16_t packetId);

    int mqttClientFormatSubscribe(MqttClientContext *context,
                                  const char *topic, MqttQosLevel qos);

    int mqttClientFormatUnsubscribe(MqttClientContext *context,
                                    const char *topic);

    int mqttClientFormatPingReq(MqttClientContext *context);
    int mqttClientFormatDisconnect(MqttClientContext *context);

#ifdef __cplusplus
}
#endif

#endif