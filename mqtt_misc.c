

#include "mqtt_client.h"
#include "mqtt_packet.h"
#include "mqtt_transport.h"
#include "mqtt_client_error_codes.h"
#include "mqtt_misc.h"
#include <pthread.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
pthread_mutex_t mqtt_misc_mutex = PTHREAD_MUTEX_INITIALIZER;

void mqttClientChangeState(MqttClientContext *context,
                           MqttClientState newState)
{

   context->state = newState;
}

int mqttClientProcessEvents(MqttClientContext *context, uint64_t timeout)
{
   int error;
   uint16_t n;

   pthread_mutex_lock(&mqtt_misc_mutex);
   error = mqttClientCheckKeepAlive(context);

   if (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE ||
          context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         error = mqttClientWaitForData(context, timeout);

         if (!error)
         {

            context->packet = context->buffer;
            context->packetPos = 0;
            context->packetLen = 0;
            context->remainingLen = 0;

            mqttClientChangeState(context, MQTT_CLIENT_STATE_RECEIVING_PACKET);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientReceivePacket(context);

         if (!error)
         {

            error = mqttClientProcessPacket(context);

            if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
            {
               if (context->packetType == MQTT_PACKET_TYPE_INVALID)
                  mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
               else
                  mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_SENT);
            }
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         if (context->packetPos < context->packetLen)
         {

            error = mqttClientSendData(context, context->packet + context->packetPos,
                                       context->packetLen - context->packetPos, &n, 0);
            context->packetPos += n;
         }
         else
         {

            uint64_t ticks = context->getuptime();
            context->keepAliveTimestamp = ticks / 1000;

            if (context->packetType == MQTT_PACKET_TYPE_INVALID)
            {

               mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
            }
            else
            {

               mqttClientChangeState(context, MQTT_CLIENT_STATE_PACKET_SENT);
            }
         }
      }
   }
   pthread_mutex_unlock(&mqtt_misc_mutex);
   return error;
}

int mqttClientCheckKeepAlive(MqttClientContext *context)
{
   int error;
   uint64_t time;
   uint64_t keepAlive;

   error = 0;

   if (context->state == MQTT_CLIENT_STATE_IDLE ||
       context->state == MQTT_CLIENT_STATE_PACKET_SENT)
   {

      if (context->settings.keepAlive != 0)
      {

         uint64_t ticks = context->getuptime();
         time = ticks / 1000;

         keepAlive = context->settings.keepAlive * 1000;

         if ((time - context->keepAliveTimestamp) > keepAlive)
         {

            error = mqttClientFormatPingReq(context);

            if (!error)
            {

               context->packetPos = 0;

               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
            }
         }
      }
   }

   return error;
}

int mqttSerializeHeader(uint8_t *buffer, uint16_t *pos, MqttPacketType type,
                        bool dup, MqttQosLevel qos, bool retain, uint16_t remainingLen)
{
   uint8_t i;
   uint8_t k;
   uint16_t n;
   MqttPacketHeader *header;

   n = *pos;

   if (remainingLen < 128)
      k = 1;
   else if (remainingLen < 16384)
      k = 2;
   else if (remainingLen < 2097152)
      k = 3;
   else if (remainingLen < 268435456)
      k = 4;
   else
      return -EIO;

   if (n < (sizeof(MqttPacketHeader) + k))
      return -EOVERFLOW;

   n -= sizeof(MqttPacketHeader) + k;

   header = (MqttPacketHeader *)(buffer + n);

   header->type = type;
   header->dup = dup;
   header->qos = qos;
   header->retain = retain;

   for (i = 0; i < k; i++)
   {

      header->length[i] = remainingLen & 0xFF;
      remainingLen >>= 7;

      if (remainingLen > 0)
         header->length[i] |= 0x80;
   }

   *pos = n;

   return 0;
}

int mqttSerializeByte(uint8_t *buffer, uint16_t bufferLen,
                      uint16_t *pos, uint8_t value)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint8_t)) > bufferLen)
      return -EOVERFLOW;

   buffer[n++] = value;

   *pos = n;

   return 0;
}

int mqttSerializeShort(uint8_t *buffer, uint16_t bufferLen,
                       uint16_t *pos, uint16_t value)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint16_t)) > bufferLen)
      return -EOVERFLOW;

   buffer[n++] = ((value)&0xFF00) >> 8;
   buffer[n++] = (value)&0x00FF;

   *pos = n;

   return 0;
}

int mqttSerializeString(uint8_t *buffer, uint16_t bufferLen,
                        uint16_t *pos, const void *string, uint16_t stringLen)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint16_t) + stringLen) > bufferLen)
      return -EOVERFLOW;

   buffer[n++] = ((stringLen)&0xFF00) >> 8;
   buffer[n++] = (stringLen)&0x00FF;

   memcpy(buffer + n, string, stringLen);

   *pos = n + stringLen;

   return 0;
}

int mqttSerializeData(uint8_t *buffer, uint16_t bufferLen,
                      uint16_t *pos, const void *data, uint16_t dataLen)
{
   uint16_t n;

   n = *pos;

   if ((n + dataLen) > bufferLen)
      return -EOVERFLOW;

   memcpy(buffer + n, data, dataLen);

   *pos = n + dataLen;

   return 0;
}

int mqttDeserializeHeader(uint8_t *buffer, uint16_t bufferLen, uint16_t *pos,
                          MqttPacketType *type, bool *dup, MqttQosLevel *qos, bool *retain, uint16_t *remainingLen)
{
   uint8_t i;
   uint16_t n;
   MqttPacketHeader *header;

   n = *pos;

   if ((n + sizeof(MqttPacketHeader)) > bufferLen)
      return -EIO;

   header = (MqttPacketHeader *)(buffer + n);

   *type = (MqttPacketType)header->type;

   *dup = header->dup;
   *qos = (MqttQosLevel)header->qos;
   *retain = header->retain;

   n += sizeof(MqttPacketHeader);

   *remainingLen = 0;

   for (i = 0; i < 4; i++)
   {

      if ((n + sizeof(uint8_t)) > bufferLen)
         return -EIO;

      n += sizeof(uint8_t);

      if (header->length[i] & 0x80)
      {

         if (i == 3)
            return -EINVAL;

         *remainingLen |= (header->length[i] & 0x7F) << (7 * i);
      }
      else
      {

         *remainingLen |= header->length[i] << (7 * i);

         break;
      }
   }

   *pos = n;

   return 0;
}

int mqttDeserializeByte(uint8_t *buffer, uint16_t bufferLen,
                        uint16_t *pos, uint8_t *value)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint8_t)) > bufferLen)
      return -EOVERFLOW;

   *value = buffer[n];

   *pos = n + sizeof(uint8_t);

   return 0;
}

int mqttDeserializeShort(uint8_t *buffer, uint16_t bufferLen,
                         uint16_t *pos, uint16_t *value)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint16_t)) > bufferLen)
      return -EOVERFLOW;

   *value = (buffer[n] << 8) | buffer[n + 1];

   *pos = n + sizeof(uint16_t);

   return 0;
}

int mqttDeserializeString(uint8_t *buffer, uint16_t bufferLen,
                          uint16_t *pos, char **string, uint16_t *stringLen)
{
   uint16_t n;

   n = *pos;

   if ((n + sizeof(uint16_t)) > bufferLen)
      return -EOVERFLOW;

   *stringLen = (buffer[n] << 8) | buffer[n + 1];

   if ((n + sizeof(uint16_t) + *stringLen) > bufferLen)
      return -EOVERFLOW;

   *string = (char *)buffer + n + 2;

   *pos = n + 2 + *stringLen;

   return 0;
}

int mqttClientCheckTimeout(MqttClientContext *context)
{
   uint8_t error;
   uint32_t time;

   uint64_t ticks = context->getuptime();
   time = ticks / 1000;

   if (time - context->startTime > context->settings.timeout)
   {

      error = -ETIME;
   }
   else
   {

      error = -EWOULDBLOCK;
   }

   return error;
}
