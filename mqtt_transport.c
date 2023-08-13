

#include "mqtt_client.h"
#include "mqtt_packet.h"
#include "mqtt_transport.h"
#include "mqtt_client_error_codes.h"
#include "mqtt_misc.h"
#include "sys/poll.h"
#include <pthread.h>
#include "stdio.h"
#include "stdint.h"
#include "string.h"
int mqttClientSendData(MqttClientContext *context,
                       const char *data, uint16_t length, uint16_t *written, uint8_t flags)
{
   int error = 0;
   uint8_t timout_count = context->settings.timeout / 1000;
   uint32_t timout_time = 1000;
   uint8_t timout_counter = 0;
   while (timout_counter < timout_count && (error == 0))
   {

      error = context->ssl_write(context->ssl, data, length);
      if (error < 0)
      {
         timout_counter++;
         usleep(timout_time * 1000);
      }
      else if (error == 0)
      {

         error = -ETIMEDOUT;
      }
   }

   if (error > 0)
   {

      *written = error;
      error = 0;
   }

   return error;
}

int mqttClientReceiveData(MqttClientContext *context,
                          char *data, uint16_t size, uint16_t *received, uint8_t flags)
{
   int error = 0;
   uint8_t error_value = 0;
   uint8_t timout_count = context->settings.timeout / 1000;
   uint32_t timout_time = 1000;
   uint8_t timout_counter = 0;
   *received = 0;
   while (timout_counter < timout_count && (error == 0))
   {

      error = context->ssl_read(context->ssl, data, size);

      if (error < 0)
      {

         timout_counter++;
         usleep(timout_time * 1000);
      }
      else if (error == 0)
      {

         error = -ETIMEDOUT;
      }
   }
   if (error > 0)
   {
      data[error] = '\0';
      *received = error;
      error = 0;
   }

   return error;
}

int mqttClientWaitForData(MqttClientContext *context, uint64_t timeout)
{
   int event;

   struct pollfd fds = {
       .fd = context->socket,
       .events = POLLIN,
   };
   event = poll(&fds, 1, timeout);
   if (event != 0)
   {
      event = 0;
      return event;
   }
   else
      return -ETIME;
}
