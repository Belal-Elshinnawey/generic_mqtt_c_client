#ifndef _MQTT_CLIENT_H
#define _MQTT_CLIENT_H

#include "mqtt_common.h"
#include <stdint.h>
#include <stdbool.h>
#include "stdio.h"
#include "string.h"
#include <inttypes.h>

#ifndef MQTT_CLIENT_DEFAULT_KEEP_ALIVE
#define MQTT_CLIENT_DEFAULT_KEEP_ALIVE 30
#endif

#ifndef MQTT_CLIENT_DEFAULT_TIMEOUT
#define MQTT_CLIENT_DEFAULT_TIMEOUT 20000
#endif

#ifndef MQTT_CLIENT_MAX_HOST_LEN
#define MQTT_CLIENT_MAX_HOST_LEN 32
#endif

#ifndef MQTT_CLIENT_MAX_URI_LEN
#define MQTT_CLIENT_MAX_URI_LEN 16
#endif

#ifndef MQTT_CLIENT_MAX_ID_LEN
#define MQTT_CLIENT_MAX_ID_LEN 32
#endif

#ifndef MQTT_CLIENT_MAX_USERNAME_LEN
#define MQTT_CLIENT_MAX_USERNAME_LEN 32
#endif

#ifndef MQTT_CLIENT_MAX_PASSWORD_LEN
#define MQTT_CLIENT_MAX_PASSWORD_LEN 50
#endif

#ifndef MQTT_CLIENT_MAX_WILL_TOPIC_LEN
#define MQTT_CLIENT_MAX_WILL_TOPIC_LEN 32
#endif

#ifndef MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN
#define MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN 32
#endif

#ifndef MQTT_CLIENT_BUFFER_SIZE
#define MQTT_CLIENT_BUFFER_SIZE 1024
#endif

#ifndef MQTT_CLIENT_PRIVATE_CONTEXT
#define MQTT_CLIENT_PRIVATE_CONTEXT
#endif

struct _MqttClientContext;
#define MqttClientContext struct _MqttClientContext

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        MQTT_CLIENT_STATE_DISCONNECTED = 0,
        MQTT_CLIENT_STATE_CONNECTING = 1,
        MQTT_CLIENT_STATE_CONNECTED = 2,
        MQTT_CLIENT_STATE_IDLE = 3,
        MQTT_CLIENT_STATE_SENDING_PACKET = 4,
        MQTT_CLIENT_STATE_PACKET_SENT = 5,
        MQTT_CLIENT_STATE_WAITING_PACKET = 6,
        MQTT_CLIENT_STATE_RECEIVING_PACKET = 7,
        MQTT_CLIENT_STATE_PACKET_RECEIVED = 8,
        MQTT_CLIENT_STATE_DISCONNECTING = 9
    } MqttClientState;

    typedef void (*MqttClientConnAckCallback)(MqttClientContext *context,
                                              uint8_t connectAckFlags, uint8_t connectReturnCode);

    typedef void (*MqttClientPublishCallback)(MqttClientContext *context,
                                              const char *topic, const uint8_t *message, uint16_t length,
                                              bool dup, MqttQosLevel qos, bool retain, uint16_t packetId);

    typedef void (*MqttClientPubAckCallback)(MqttClientContext *context,
                                             uint16_t packetId);

    typedef void (*MqttClientPubRecCallback)(MqttClientContext *context,
                                             uint16_t packetId);

    typedef void (*MqttClientPubRelCallback)(MqttClientContext *context,
                                             uint16_t packetId);

    typedef void (*MqttClientPubCompCallback)(MqttClientContext *context,
                                              uint16_t packetId);

    typedef void (*MqttClientSubAckCallback)(MqttClientContext *context,
                                             uint16_t packetId);

    typedef void (*MqttClientUnsubAckCallback)(MqttClientContext *context,
                                               uint16_t packetId);

    typedef void (*MqttClientPingRespCallback)(MqttClientContext *context);

    typedef struct
    {
        char topic[MQTT_CLIENT_MAX_WILL_TOPIC_LEN + 1];
        uint8_t payload[MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN];
        uint16_t length;
        MqttQosLevel qos;
        bool retain;
    } MqttClientWillMessage;

    typedef struct
    {
        MqttClientConnAckCallback connAckCallback;
        MqttClientPublishCallback publishCallback;
        MqttClientPubAckCallback pubAckCallback;
        MqttClientPubAckCallback pubRecCallback;
        MqttClientPubAckCallback pubRelCallback;
        MqttClientPubAckCallback pubCompCallback;
        MqttClientPubAckCallback subAckCallback;
        MqttClientPubAckCallback unsubAckCallback;
        MqttClientPingRespCallback pingRespCallback;
    } MqttClientCallbacks;

    typedef struct
    {
        MqttVersion version;
        MqttTransportProtocol transportProtocol;
        uint16_t keepAlive;
        uint16_t timeout;
        char clientId[MQTT_CLIENT_MAX_ID_LEN + 1];
        char username[MQTT_CLIENT_MAX_USERNAME_LEN + 1];
        char password[MQTT_CLIENT_MAX_PASSWORD_LEN + 1];
        MqttClientWillMessage willMessage;
    } MqttClientSettings;

    typedef int (*ssl_read_callback_t)(void *ssl, unsigned char *buf, size_t len);
    typedef int (*ssl_write_callback_t)(void *ssl, const unsigned char *buf, size_t len);
    typedef int64_t (*get_uptime_callback_t)();
    struct _MqttClientContext
    {
        get_uptime_callback_t getuptime;
        MqttClientSettings settings;
        MqttClientCallbacks callbacks;
        MqttClientState state;
        int socket;
        void *ssl;
        ssl_read_callback_t ssl_read;
        ssl_write_callback_t ssl_write;
        uint64_t startTime;
        uint64_t keepAliveTimestamp;
        uint8_t buffer[MQTT_CLIENT_BUFFER_SIZE];
        uint8_t *packet;
        uint16_t packetPos;
        uint16_t packetLen;
        MqttPacketType packetType;
        uint16_t packetId;
        uint16_t remainingLen;
        MQTT_CLIENT_PRIVATE_CONTEXT
    };

    int mqttClientInit(MqttClientContext *context);
    void mqttClientInitCallbacks(MqttClientCallbacks *callbacks);

    int mqttClientRegisterCallbacks(MqttClientContext *context,
                                    const MqttClientCallbacks *callbacks);

    int mqttClientSetVersion(MqttClientContext *context, MqttVersion version);

    int mqttClientSetTransportProtocol(MqttClientContext *context,
                                       MqttTransportProtocol transportProtocol);

    int mqttClientRegisterPublishCallback(MqttClientContext *context,
                                          MqttClientPublishCallback callback);

    int mqttClientSetTimeout(MqttClientContext *context, uint64_t timeout);

    int mqttClientSetSocket(MqttClientContext *context, int socketfd);

    int mqttClientSetSSL(MqttClientContext *context, void *ssl, ssl_read_callback_t read_fn, ssl_write_callback_t write_fn);

    int mqttClientSetKeepAlive(MqttClientContext *context, uint16_t keepAlive);

    int mqttClientSetIdentifier(MqttClientContext *context,
                                const char *clientId);

    int mqttClientSetAuthInfo(MqttClientContext *context,
                              const char *username, const char *password);

    int mqttClientSetWillMessage(MqttClientContext *context, const char *topic,
                                 const void *message, uint16_t length, MqttQosLevel qos, bool retain);

    int mqttClientConnect(MqttClientContext *context, bool cleanSession);

    int mqttClientPublish(MqttClientContext *context,
                          const char *topic, const void *message, uint16_t length,
                          MqttQosLevel qos, bool retain, uint16_t *packetId);

    int mqttClientSubscribe(MqttClientContext *context,
                            const char *topic, MqttQosLevel qos, uint16_t *packetId);

    int mqttClientUnsubscribe(MqttClientContext *context,
                              const char *topic, uint16_t *packetId);

    int mqttClientPing(MqttClientContext *context, uint64_t *rtt);

    int mqttClientTask(MqttClientContext *context, uint64_t timeout);

    int mqttClientDisconnect(MqttClientContext *context);
    int mqttClientClose(MqttClientContext *context);

    void mqttClientDeinit(MqttClientContext *context);
    int mqttClientSetuptimeCallback(MqttClientContext *context, get_uptime_callback_t uptime_cb);

#ifdef __cplusplus
}
#endif

#endif