#ifndef _MQTT_COMMON_H
#define _MQTT_COMMON_H
#include <stdint.h>
#include "stdio.h"
#include "string.h"

#define MQTT_PROTOCOL_NAME_3_1 "MQIsdp"

#define MQTT_PROTOCOL_NAME_3_1_1 "MQTT"

#define MQTT_MIN_HEADER_SIZE 2

#define MQTT_MAX_HEADER_SIZE 5

#ifdef __cplusplus
extern "C"
{
#endif

   typedef enum
   {
      MQTT_VERSION_3_1 = 3,
      MQTT_VERSION_3_1_1 = 4
   } MqttVersion;

   typedef enum
   {
      MQTT_TRANSPORT_PROTOCOL_TCP = 1,
      MQTT_TRANSPORT_PROTOCOL_TLS = 2,
      MQTT_TRANSPORT_PROTOCOL_WS = 3,
      MQTT_TRANSPORT_PROTOCOL_WSS = 4,
   } MqttTransportProtocol;

   typedef enum
   {
      MQTT_QOS_LEVEL_0 = 0,
      MQTT_QOS_LEVEL_1 = 1,
      MQTT_QOS_LEVEL_2 = 2
   } MqttQosLevel;

   typedef enum
   {
      MQTT_PACKET_TYPE_INVALID = 0,
      MQTT_PACKET_TYPE_CONNECT = 1,
      MQTT_PACKET_TYPE_CONNACK = 2,
      MQTT_PACKET_TYPE_PUBLISH = 3,
      MQTT_PACKET_TYPE_PUBACK = 4,
      MQTT_PACKET_TYPE_PUBREC = 5,
      MQTT_PACKET_TYPE_PUBREL = 6,
      MQTT_PACKET_TYPE_PUBCOMP = 7,
      MQTT_PACKET_TYPE_SUBSCRIBE = 8,
      MQTT_PACKET_TYPE_SUBACK = 9,
      MQTT_PACKET_TYPE_UNSUBSCRIBE = 10,
      MQTT_PACKET_TYPE_UNSUBACK = 11,
      MQTT_PACKET_TYPE_PINGREQ = 12,
      MQTT_PACKET_TYPE_PINGRESP = 13,
      MQTT_PACKET_TYPE_DISCONNECT = 14
   } MqttPacketType;

   typedef enum
   {
      MQTT_CONNECT_FLAG_CLEAN_SESSION = 0x02,
      MQTT_CONNECT_FLAG_WILL = 0x04,
      MQTT_CONNECT_FLAG_WILL_QOS_0 = 0x00,
      MQTT_CONNECT_FLAG_WILL_QOS_1 = 0x08,
      MQTT_CONNECT_FLAG_WILL_QOS_2 = 0x10,
      MQTT_CONNECT_FLAG_WILL_RETAIN = 0x20,
      MQTT_CONNECT_FLAG_PASSWORD = 0x40,
      MQTT_CONNECT_FLAG_USERNAME = 0x80
   } MqttConnectFlags;

   typedef enum
   {
      MQTT_CONNECT_ACK_FLAG_SESSION_PRESENT = 0x01
   } MqttConnectAckFlags;

   typedef enum
   {
      MQTT_CONNECT_RET_CODE_ACCEPTED = 0,
      MQTT_CONNECT_RET_CODE_UNACCEPTABLE_VERSION = 1,
      MQTT_CONNECT_RET_CODE_ID_REJECTED = 2,
      MQTT_CONNECT_RET_CODE_SERVER_UNAVAILABLE = 3,
      MQTT_CONNECT_RET_CODE_BAD_USER_NAME = 4,
      MQTT_CONNECT_RET_CODE_NOT_AUTHORIZED = 5
   } MqttConnectRetCode;

   typedef struct
   {
      uint8_t retain : 1;
      uint8_t qos : 2;
      uint8_t dup : 1;
      uint8_t type : 4;
      uint8_t length[];
   } MqttPacketHeader;

   typedef struct
   {
      uint16_t length;
      uint8_t data[];
   } MqttString;

#ifdef __cplusplus
}
#endif

#endif