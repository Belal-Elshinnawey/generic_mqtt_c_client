#ifndef _MQTT_INTERFACE_CLIENT_CONFIG_H__
#define _MQTT_INTERFACE_CLIENT_CONFIG_H__
#ifdef __cplusplus
extern "C"
{
#endif
#include <inttypes.h>

#define SERVER_ADDR ""
#define SERVER_PORT "8885"
#define TLS_SNI_HOSTNAME "test.mosquitto.org"

    void start_client_thread(char *set_client_id, char *set_username, char *set_password, char *topic);
    int publish_mqtt_message(char *message, int length);
#endif
#ifdef __cplusplus
}
#endif
