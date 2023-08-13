

#include "mqtt_client.h"
#include "mqtt_packet.h"
#include "mqtt_client_error_codes.h"
#include "mqtt_transport.h"
#include "mqtt_misc.h"
#include "stdio.h"
#include "stdint.h"
#include <pthread.h>
#include "string.h"

const char *const mqttPacketLabel[16] =
    {
        "Reserved",
        "CONNECT",
        "CONNACK",
        "PUBLISH",
        "PUBACK",
        "PUBREC",
        "PUBREL",
        "PUBCOMP",
        "SUBSCRIBE",
        "SUBACK",
        "UNSUBSCRIBE",
        "UNSUBACK",
        "PINGREQ",
        "PINGRESP",
        "DISCONNECT",
        "Reserved"};

int mqttClientReceivePacket(MqttClientContext *context)
{
    int error;
    uint16_t n;
    uint8_t value;

    error = 0;

    while (1)
    {

        if (context->packetLen == 0)
        {

            error = mqttClientReceiveData(context, &value, sizeof(uint8_t), &n, 0);

            if (!error)
            {

                context->packet[context->packetPos] = value;

                if (context->packetPos > 0)
                {

                    if (value & 0x80)
                    {

                        if (context->packetPos < 4)
                        {

                            context->remainingLen |= (value & 0x7F) << (7 * (context->packetPos - 1));
                        }
                        else
                        {

                            error = -EINVAL;
                        }
                    }
                    else
                    {

                        context->remainingLen |= value << (7 * (context->packetPos - 1));

                        context->packetLen = context->packetPos + 1 + context->remainingLen;

                        if (context->packetLen > MQTT_CLIENT_BUFFER_SIZE)
                            error = -EINVAL;
                    }
                }

                context->packetPos++;
            }
        }

        else
        {

            if (context->packetPos < context->packetLen)
            {

                error = mqttClientReceiveData(context, context->packet + context->packetPos,
                                              context->packetLen - context->packetPos, &n, 0);

                context->packetPos += n;
            }
            else
            {

                break;
            }
        }

        if (error)
            break;
    }

    return error;
}

int mqttClientProcessPacket(MqttClientContext *context)
{
    int error;
    bool dup;
    bool retain;
    uint16_t remainingLen;
    MqttQosLevel qos;
    MqttPacketType type;

    context->packetPos = 0;

    error = mqttDeserializeHeader(context->packet, context->packetLen,
                                  &context->packetPos, &type, &dup, &qos, &retain, &remainingLen);

    if (error)
        return error;

    switch (type)
    {

    case MQTT_PACKET_TYPE_CONNACK:

        error = mqttClientProcessConnAck(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_PUBLISH:

        error = mqttClientProcessPublish(context, dup, qos, retain, remainingLen);

        break;

    case MQTT_PACKET_TYPE_PUBACK:

        error = mqttClientProcessPubAck(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_PUBREC:

        error = mqttClientProcessPubRec(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_PUBREL:

        error = mqttClientProcessPubRel(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_PUBCOMP:

        error = mqttClientProcessPubComp(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_SUBACK:

        error = mqttClientProcessSubAck(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_UNSUBACK:

        error = mqttClientProcessUnsubAck(context, dup, qos, retain, remainingLen);
        break;

    case MQTT_PACKET_TYPE_PINGRESP:

        error = mqttClientProcessPingResp(context, dup, qos, retain, remainingLen);
        break;

    default:

        error = -EINVAL;
    }
    usleep(1000 * 100);

    return error;
}

int mqttClientProcessConnAck(MqttClientContext *context,
                             bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint8_t connectAckFlags;
    uint8_t connectReturnCode;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeByte(context->packet, context->packetLen,
                                &context->packetPos, &connectAckFlags);

    if (error)
        return error;

    error = mqttDeserializeByte(context->packet, context->packetLen,
                                &context->packetPos, &connectReturnCode);

    if (error)
        return error;

    if (context->callbacks.connAckCallback != NULL)
    {

        context->callbacks.connAckCallback(context,
                                           connectAckFlags, connectReturnCode);
    }

    if (connectReturnCode != MQTT_CONNECT_RET_CODE_ACCEPTED)
        return -ECONNREFUSED;

    if (context->packetType == MQTT_PACKET_TYPE_CONNECT)
        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return 0;
}

int mqttClientProcessPublish(MqttClientContext *context,
                             bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;
    char *topic;
    uint16_t topicLen;
    uint8_t *message;
    uint16_t messageLen;

    error = mqttDeserializeString(context->packet, context->packetLen,
                                  &context->packetPos, &topic, &topicLen);

    if (error)
        return error;

    if (qos != MQTT_QOS_LEVEL_0)
    {

        error = mqttDeserializeShort(context->packet, context->packetLen,
                                     &context->packetPos, &packetId);

        if (error)
            return error;
    }
    else
    {

        packetId = 0;
    }

    message = context->packet + context->packetPos;

    messageLen = context->packetLen - context->packetPos;

    memmove(topic - 1, topic, topicLen);

    topic[topicLen - 1] = '\0';

    topic--;

    if (context->callbacks.publishCallback != NULL)
    {

        context->callbacks.publishCallback(context, topic,
                                           message, messageLen, dup, qos, retain, packetId);
    }

    if (qos == MQTT_QOS_LEVEL_1)
    {

        error = mqttClientFormatPubAck(context, packetId);

        if (!error)
        {

            context->packetPos = 0;

            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
        }
    }
    else if (qos == MQTT_QOS_LEVEL_2)
    {

        error = mqttClientFormatPubRec(context, packetId);

        if (!error)
        {

            context->packetPos = 0;

            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
        }
    }

    return error;
}

int mqttClientProcessPubAck(MqttClientContext *context,
                            bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.pubAckCallback != NULL)
    {

        context->callbacks.pubAckCallback(context, packetId);
    }

    if (context->packetType == MQTT_PACKET_TYPE_PUBLISH && context->packetId == packetId)

        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return error;
}

int mqttClientProcessPubRec(MqttClientContext *context,
                            bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.pubRecCallback != NULL)
    {

        context->callbacks.pubRecCallback(context, packetId);
    }

    error = mqttClientFormatPubRel(context, packetId);

    if (!error)
    {

        context->packetPos = 0;

        mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
    }

    return error;
}

int mqttClientProcessPubRel(MqttClientContext *context,
                            bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_1 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.pubRelCallback != NULL)
    {

        context->callbacks.pubRelCallback(context, packetId);
    }

    error = mqttClientFormatPubComp(context, packetId);

    if (!error)
    {

        context->packetPos = 0;

        mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
    }

    return error;
}

int mqttClientProcessPubComp(MqttClientContext *context,
                             bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.pubCompCallback != NULL)
    {

        context->callbacks.pubCompCallback(context, packetId);
    }

    if (context->packetType == MQTT_PACKET_TYPE_PUBLISH && context->packetId == packetId)
        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return 0;
}

int mqttClientProcessSubAck(MqttClientContext *context,
                            bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.subAckCallback != NULL)
    {

        context->callbacks.subAckCallback(context, packetId);
    }

    if (context->packetType == MQTT_PACKET_TYPE_SUBSCRIBE && context->packetId == packetId)
        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return 0;
}

int mqttClientProcessUnsubAck(MqttClientContext *context,
                              bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
    int error;
    uint16_t packetId;

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    error = mqttDeserializeShort(context->packet, context->packetLen,
                                 &context->packetPos, &packetId);

    if (error)
        return error;

    if (context->callbacks.unsubAckCallback != NULL)
    {

        context->callbacks.unsubAckCallback(context, packetId);
    }

    if (context->packetType == MQTT_PACKET_TYPE_UNSUBSCRIBE && context->packetId == packetId)
        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return 0;
}

int mqttClientProcessPingResp(MqttClientContext *context,
                              bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{

    if (dup != false && qos != MQTT_QOS_LEVEL_0 && retain != false)
        return -EINVAL;

    if (context->callbacks.pingRespCallback != NULL)
    {

        context->callbacks.pingRespCallback(context);
    }

    if (context->packetType == MQTT_PACKET_TYPE_PINGREQ)
        mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_RECEIVED);

    return 0;
}

int mqttClientFormatConnect(MqttClientContext *context,
                            bool cleanSession)
{
    int error;
    uint16_t n;
    uint8_t connectFlags;
    MqttClientWillMessage *willMessage;

    n = MQTT_MAX_HEADER_SIZE;

    if (context->settings.version == MQTT_VERSION_3_1)
    {

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, MQTT_PROTOCOL_NAME_3_1, strlen(MQTT_PROTOCOL_NAME_3_1));
    }
    else if (context->settings.version == MQTT_VERSION_3_1_1)
    {

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, MQTT_PROTOCOL_NAME_3_1_1, strlen(MQTT_PROTOCOL_NAME_3_1_1));
    }
    else
    {

        error = -EINVAL;
    }

    if (error)
        return error;

    error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                              &n, context->settings.version);

    if (error)
        return error;

    connectFlags = 0;

    if (cleanSession)
        connectFlags |= MQTT_CONNECT_FLAG_CLEAN_SESSION;

    if (context->settings.clientId[0] == '\0')
        connectFlags |= MQTT_CONNECT_FLAG_CLEAN_SESSION;

    willMessage = &context->settings.willMessage;

    if (willMessage->topic[0] != '\0')
    {

        connectFlags |= MQTT_CONNECT_FLAG_WILL;

        if (willMessage->qos == MQTT_QOS_LEVEL_1)
            connectFlags |= MQTT_CONNECT_FLAG_WILL_QOS_1;
        else if (willMessage->qos == MQTT_QOS_LEVEL_2)
            connectFlags |= MQTT_CONNECT_FLAG_WILL_QOS_2;

        if (willMessage->retain)
            connectFlags |= MQTT_CONNECT_FLAG_WILL_RETAIN;
    }

    if (context->settings.username[0] != '\0')
        connectFlags |= MQTT_CONNECT_FLAG_USERNAME;

    if (context->settings.password[0] != '\0')
        connectFlags |= MQTT_CONNECT_FLAG_PASSWORD;

    error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                              &n, connectFlags);

    if (error)
        return error;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, context->settings.keepAlive);

    if (error)
        return error;

    error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                &n, context->settings.clientId, strlen(context->settings.clientId));

    if (error)
        return error;

    if (willMessage->topic[0] != '\0')
    {

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, willMessage->topic, strlen(willMessage->topic));

        if (error)
            return error;

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, willMessage->payload, willMessage->length);

        if (error)
            return error;
    }

    if (context->settings.username[0] != '\0')
    {

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, context->settings.username, strlen(context->settings.username));

        if (error)
            return error;
    }

    if (context->settings.password[0] != '\0')
    {

        error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                    &n, context->settings.password, strlen(context->settings.password));

        if (error)
            return error;
    }

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_CONNECT,
                                false, MQTT_QOS_LEVEL_0, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPublish(MqttClientContext *context, const char *topic,
                            const void *message, uint16_t length, MqttQosLevel qos, bool retain)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                &n, topic, strlen(topic));

    if (error)
        return error;

    if (qos != MQTT_QOS_LEVEL_0)
    {

        context->packetId++;

        error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                   &n, context->packetId);

        if (error)
            return error;
    }

    error = mqttSerializeData(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                              &n, message, length);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBLISH,
                                false, qos, retain, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPubAck(MqttClientContext *context, uint16_t packetId)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, packetId);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBACK,
                                false, MQTT_QOS_LEVEL_0, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPubRec(MqttClientContext *context, uint16_t packetId)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, packetId);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBREC,
                                false, MQTT_QOS_LEVEL_0, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPubRel(MqttClientContext *context, uint16_t packetId)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, packetId);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBREL,
                                false, MQTT_QOS_LEVEL_1, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPubComp(MqttClientContext *context, uint16_t packetId)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, packetId);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PUBCOMP,
                                false, MQTT_QOS_LEVEL_0, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatSubscribe(MqttClientContext *context,
                              const char *topic, MqttQosLevel qos)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    context->packetId++;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, context->packetId);

    if (error)
        return error;

    error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                &n, topic, strlen(topic));

    if (error)
        return error;

    error = mqttSerializeByte(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                              &n, qos);

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_SUBSCRIBE,
                                false, MQTT_QOS_LEVEL_1, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatUnsubscribe(MqttClientContext *context,
                                const char *topic)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    context->packetId++;

    error = mqttSerializeShort(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                               &n, context->packetId);

    if (error)
        return error;

    error = mqttSerializeString(context->buffer, MQTT_CLIENT_BUFFER_SIZE,
                                &n, topic, strlen(topic));

    if (error)
        return error;

    context->packetLen = n - MQTT_MAX_HEADER_SIZE;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_UNSUBSCRIBE,
                                false, MQTT_QOS_LEVEL_1, false, context->packetLen);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen += MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatPingReq(MqttClientContext *context)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_PINGREQ,
                                false, MQTT_QOS_LEVEL_0, false, 0);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen = MQTT_MAX_HEADER_SIZE - n;

    return 0;
}

int mqttClientFormatDisconnect(MqttClientContext *context)
{
    int error;
    uint16_t n;

    n = MQTT_MAX_HEADER_SIZE;

    error = mqttSerializeHeader(context->buffer, &n, MQTT_PACKET_TYPE_DISCONNECT,
                                false, MQTT_QOS_LEVEL_0, false, 0);

    if (error)
        return error;

    context->packet = context->buffer + n;

    context->packetLen = MQTT_MAX_HEADER_SIZE - n;

    return 0;
}
