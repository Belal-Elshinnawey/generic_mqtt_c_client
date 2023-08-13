
#include "mqtt_client.h"
#include "mqtt_packet.h"
#include "mqtt_client_error_codes.h"
#include "mqtt_transport.h"
#include "mqtt_misc.h"
#include "stdio.h"
#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include <inttypes.h>
#include <pthread.h>
pthread_mutex_t mqtt_mutex = PTHREAD_MUTEX_INITIALIZER;
int mqttClientInit(MqttClientContext *context)
{

   if (context == NULL)
   {
      return -EINVAL;
   }
   pthread_mutex_init(&mqtt_mutex, NULL);
   memset(context, 0, sizeof(MqttClientContext));

   context->settings.version = MQTT_VERSION_3_1_1;

   context->settings.transportProtocol = MQTT_TRANSPORT_PROTOCOL_TCP;

   context->settings.keepAlive = MQTT_CLIENT_DEFAULT_KEEP_ALIVE;

   context->settings.timeout = MQTT_CLIENT_DEFAULT_TIMEOUT;

   context->state = MQTT_CLIENT_STATE_DISCONNECTED;

   context->packetId = 0;

   return 0;
}

void mqttClientInitCallbacks(MqttClientCallbacks *callbacks)
{

   memset(callbacks, 0, sizeof(MqttClientCallbacks));
}

int mqttClientRegisterCallbacks(MqttClientContext *context,
                                const MqttClientCallbacks *callbacks)
{

   if (context == NULL)
      return -EINVAL;

   context->callbacks = *callbacks;

   return 0;
}

int mqttClientSetVersion(MqttClientContext *context, MqttVersion version)
{

   if (context == NULL)
      return -EINVAL;

   context->settings.version = version;

   return 0;
}

int mqttClientSetTransportProtocol(MqttClientContext *context,
                                   MqttTransportProtocol transportProtocol)
{

   if (context == NULL)
      return -EINVAL;

   context->settings.transportProtocol = transportProtocol;

   return 0;
}

int mqttClientRegisterPublishCallback(MqttClientContext *context,
                                      MqttClientPublishCallback callback)
{

   if (context == NULL)
      return -EINVAL;

   context->callbacks.publishCallback = callback;

   return 0;
}

int mqttClientSetTimeout(MqttClientContext *context, uint64_t timeout)
{

   if (context == NULL)
      return -EINVAL;

   context->settings.timeout = timeout;

   return 0;
}

int mqttClientSetuptimeCallback(MqttClientContext *context, get_uptime_callback_t uptime_cb)
{
   if (context == NULL)
      return -EINVAL;
   context->getuptime = uptime_cb;
   return 0;
}

int mqttClientSetSocket(MqttClientContext *context, int socketfd)
{

   if (context == NULL)
      return -EINVAL;

   context->socket = socketfd;

   return 0;
}

int mqttClientSetSSL(MqttClientContext *context, void *ssl, ssl_read_callback_t read_fn, ssl_write_callback_t write_fn)
{

   if (context == NULL)
      return -EINVAL;
   context->ssl = ssl;
   context->ssl_read = read_fn;
   context->ssl_write = write_fn;

   return 0;
}

int mqttClientSetKeepAlive(MqttClientContext *context, uint16_t keepAlive)
{

   if (context == NULL)
      return -EINVAL;

   context->settings.keepAlive = keepAlive;

   return 0;
}

int mqttClientSetIdentifier(MqttClientContext *context,
                            const char *clientId)
{

   if (context == NULL)
   {
      return -EINVAL;
   }
   if (clientId == NULL)
   {
      return -EINVAL;
   }

   if (strlen(clientId) > MQTT_CLIENT_MAX_ID_LEN)
   {
      return -EINVAL;
   }

   strcpy(context->settings.clientId, clientId);

   return 0;
}

int mqttClientSetAuthInfo(MqttClientContext *context,
                          const char *username, const char *password)
{

   if (context == NULL || username == NULL || password == NULL)
   {
      return -EINVAL;
   }

   if (strlen(username) > MQTT_CLIENT_MAX_USERNAME_LEN)
   {
      return -EINVAL;
   }

   strcpy(context->settings.username, username);

   if (strlen(password) > MQTT_CLIENT_MAX_PASSWORD_LEN)
   {
      return -EINVAL;
   }

   strcpy(context->settings.password, password);

   return 0;
}

int mqttClientSetWillMessage(MqttClientContext *context, const char *topic,
                             const void *message, uint16_t length, MqttQosLevel qos, bool retain)
{
   MqttClientWillMessage *willMessage;

   if (context == NULL || topic == NULL)
      return -EINVAL;

   if (strlen(topic) > MQTT_CLIENT_MAX_WILL_TOPIC_LEN)
      return -EINVAL;

   willMessage = &context->settings.willMessage;

   strcpy(willMessage->topic, topic);

   if (length > 0)
   {

      if (message == NULL)
         return -EINVAL;

      if (strlen(message) > MQTT_CLIENT_MAX_WILL_PAYLOAD_LEN)
         return -EINVAL;

      memcpy(willMessage->payload, message, length);
   }

   willMessage->length = length;

   willMessage->qos = qos;

   willMessage->retain = retain;

   return 0;
}

int mqttClientConnect(MqttClientContext *context, bool cleanSession)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);

   if (context == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_DISCONNECTED)
      {

         if (!error)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_CONNECTING);

            uint64_t ticks = context->getuptime();
            context->startTime = ticks / 1000;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_CONNECTING)
      {
         if (!error)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_CONNECTED);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_CONNECTED)
      {

         error = mqttClientFormatConnect(context, cleanSession);

         if (!error)
         {

            context->packetType = MQTT_PACKET_TYPE_CONNECT;

            context->packetPos = 0;

            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {

         context->packetType = MQTT_PACKET_TYPE_INVALID;

         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         break;
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }

   if (error != 0 && error != -EWOULDBLOCK)
   {

      mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientPublish(MqttClientContext *context,
                      const char *topic, const void *message, uint16_t length,
                      MqttQosLevel qos, bool retain, uint16_t *packetId)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL || topic == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }
   if (message == NULL && length != 0)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         if (context->packetType == MQTT_PACKET_TYPE_INVALID)
         {

            error = mqttClientFormatPublish(context, topic, message,
                                            length, qos, retain);

            if (!error)
            {

               if (packetId != NULL)
                  *packetId = context->packetId;

               context->packetType = MQTT_PACKET_TYPE_PUBLISH;

               context->packetPos = 0;

               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);

               uint64_t ticks = context->getuptime();
               context->startTime = ticks / 1000;
            }
         }
         else
         {

            context->packetType = MQTT_PACKET_TYPE_INVALID;

            break;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         if (packetId != NULL)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {

            if (qos == MQTT_QOS_LEVEL_0)
            {

               mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
            }
            else
            {

               error = mqttClientProcessEvents(context, context->settings.timeout);
            }
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {

         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientSubscribe(MqttClientContext *context,
                        const char *topic, MqttQosLevel qos, uint16_t *packetId)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL || topic == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         if (context->packetType == MQTT_PACKET_TYPE_INVALID)
         {

            error = mqttClientFormatSubscribe(context, topic, qos);

            if (!error)
            {

               if (packetId != NULL)
                  *packetId = context->packetId;

               context->packetType = MQTT_PACKET_TYPE_SUBSCRIBE;

               context->packetPos = 0;

               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);

               uint64_t ticks = context->getuptime();
               context->startTime = ticks / 1000;
            }
         }
         else
         {

            context->packetType = MQTT_PACKET_TYPE_INVALID;

            break;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         if (packetId != NULL)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {

            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {

         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientUnsubscribe(MqttClientContext *context,
                          const char *topic, uint16_t *packetId)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL || topic == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         if (context->packetType == MQTT_PACKET_TYPE_INVALID)
         {

            error = mqttClientFormatUnsubscribe(context, topic);

            if (!error)
            {

               if (packetId != NULL)
                  *packetId = context->packetId;

               context->packetType = MQTT_PACKET_TYPE_UNSUBSCRIBE;

               context->packetPos = 0;

               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);

               uint64_t ticks = context->getuptime();
               context->startTime = ticks / 1000;
            }
         }
         else
         {

            context->packetType = MQTT_PACKET_TYPE_INVALID;

            break;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         if (packetId != NULL)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
         else
         {

            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {

         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientPing(MqttClientContext *context, uint64_t *rtt)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         if (context->packetType == MQTT_PACKET_TYPE_INVALID)
         {

            error = mqttClientFormatPingReq(context);

            if (!error)
            {

               context->packetType = MQTT_PACKET_TYPE_PINGREQ;

               context->packetPos = 0;

               mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);

               uint64_t ticks = context->getuptime();
               context->startTime = ticks / 1000;
            }
         }
         else
         {

            context->packetType = MQTT_PACKET_TYPE_INVALID;

            break;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         if (rtt != NULL)
         {

            error = mqttClientProcessEvents(context, context->settings.timeout);
         }
         else
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_RECEIVING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_RECEIVED)
      {

         if (rtt != NULL)
         {

            uint64_t ticks = context->getuptime();
            *rtt = (ticks / 1000) - context->startTime;
         }

         mqttClientChangeState(context, MQTT_CLIENT_STATE_IDLE);
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientTask(MqttClientContext *context, uint64_t timeout)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = mqttClientProcessEvents(context, timeout);

   if (error == -ETIMEDOUT)
   {

      error = 0;
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientDisconnect(MqttClientContext *context)
{
   int error;
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   error = 0;

   while (!error)
   {

      if (context->state == MQTT_CLIENT_STATE_IDLE)
      {

         error = mqttClientFormatDisconnect(context);

         if (!error)
         {

            context->packetType = MQTT_PACKET_TYPE_DISCONNECT;

            context->packetPos = 0;

            mqttClientChangeState(context, MQTT_CLIENT_STATE_SENDING_PACKET);

            uint64_t ticks = context->getuptime();
            context->startTime = ticks / 1000;
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_SENDING_PACKET)
      {

         error = mqttClientProcessEvents(context, context->settings.timeout);
      }
      else if (context->state == MQTT_CLIENT_STATE_PACKET_SENT)
      {

         mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTING);
      }
      else if (context->state == MQTT_CLIENT_STATE_DISCONNECTING)
      {

         if (!error)
         {

            mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
         }
      }
      else if (context->state == MQTT_CLIENT_STATE_DISCONNECTED)
      {

         break;
      }
      else
      {

         error = -ENOTCONN;
      }
   }

   if (error == -EWOULDBLOCK || error == -ETIMEDOUT)
   {

      error = mqttClientCheckTimeout(context);
   }

   if (error != 0 && error != -EWOULDBLOCK)
   {

      mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
   }
   pthread_mutex_unlock(&mqtt_mutex);
   return error;
}

int mqttClientClose(MqttClientContext *context)
{
   pthread_mutex_lock(&mqtt_mutex);
   if (context == NULL)
   {
      pthread_mutex_unlock(&mqtt_mutex);
      return -EINVAL;
   }

   mqttClientChangeState(context, MQTT_CLIENT_STATE_DISCONNECTED);
   pthread_mutex_unlock(&mqtt_mutex);
   return 0;
}

void mqttClientDeinit(MqttClientContext *context)
{
   if (context != NULL)
   {
      pthread_mutex_lock(&mqtt_mutex);
      memset(context, 0, sizeof(MqttClientContext));
      pthread_mutex_unlock(&mqtt_mutex);
      pthread_mutex_destroy(&mqtt_mutex);
   }
}
