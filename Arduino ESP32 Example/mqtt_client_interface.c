

#include <time.h>
#include <inttypes.h>
#include "mqtt_client_interface.h"
#include "mqtt_client_error_codes.h"
#include "mqtt_client.h"
#include <fcntl.h>
#include <sys/poll.h>
#include <esp_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "mbedtls/platform.h"
#include "mbedtls/net_sockets.h"
#include "mbedtls/esp_debug.h"
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

#include "esp_crt_bundle.h"

#define TAG "TAG"
unsigned char cert_file[] = {
    "-----BEGIN CERTIFICATE-----\n\
MIIEAzCCAuugAwIBAgIUBY1hlCGvdj4NhBXkZ/uLUZNILAwwDQYJKoZIhvcNAQEL\n\
BQAwgZAxCzAJBgNVBAYTAkdCMRcwFQYDVQQIDA5Vbml0ZWQgS2luZ2RvbTEOMAwG\n\
A1UEBwwFRGVyYnkxEjAQBgNVBAoMCU1vc3F1aXR0bzELMAkGA1UECwwCQ0ExFjAU\n\
BgNVBAMMDW1vc3F1aXR0by5vcmcxHzAdBgkqhkiG9w0BCQEWEHJvZ2VyQGF0Y2hv\n\
by5vcmcwHhcNMjAwNjA5MTEwNjM5WhcNMzAwNjA3MTEwNjM5WjCBkDELMAkGA1UE\n\
BhMCR0IxFzAVBgNVBAgMDlVuaXRlZCBLaW5nZG9tMQ4wDAYDVQQHDAVEZXJieTES\n\
MBAGA1UECgwJTW9zcXVpdHRvMQswCQYDVQQLDAJDQTEWMBQGA1UEAwwNbW9zcXVp\n\
dHRvLm9yZzEfMB0GCSqGSIb3DQEJARYQcm9nZXJAYXRjaG9vLm9yZzCCASIwDQYJ\n\
KoZIhvcNAQEBBQADggEPADCCAQoCggEBAME0HKmIzfTOwkKLT3THHe+ObdizamPg\n\
UZmD64Tf3zJdNeYGYn4CEXbyP6fy3tWc8S2boW6dzrH8SdFf9uo320GJA9B7U1FW\n\
Te3xda/Lm3JFfaHjkWw7jBwcauQZjpGINHapHRlpiCZsquAthOgxW9SgDgYlGzEA\n\
s06pkEFiMw+qDfLo/sxFKB6vQlFekMeCymjLCbNwPJyqyhFmPWwio/PDMruBTzPH\n\
3cioBnrJWKXc3OjXdLGFJOfj7pP0j/dr2LH72eSvv3PQQFl90CZPFhrCUcRHSSxo\n\
E6yjGOdnz7f6PveLIB574kQORwt8ePn0yidrTC1ictikED3nHYhMUOUCAwEAAaNT\n\
MFEwHQYDVR0OBBYEFPVV6xBUFPiGKDyo5V3+Hbh4N9YSMB8GA1UdIwQYMBaAFPVV\n\
6xBUFPiGKDyo5V3+Hbh4N9YSMA8GA1UdEwEB/wQFMAMBAf8wDQYJKoZIhvcNAQEL\n\
BQADggEBAGa9kS21N70ThM6/Hj9D7mbVxKLBjVWe2TPsGfbl3rEDfZ+OKRZ2j6AC\n\
6r7jb4TZO3dzF2p6dgbrlU71Y/4K0TdzIjRj3cQ3KSm41JvUQ0hZ/c04iGDg/xWf\n\
+pp58nfPAYwuerruPNWmlStWAXf0UTqRtg4hQDWBuUFDJTuWuuBvEXudz74eh/wK\n\
sMwfu1HFvjy5Z0iMDU8PUDepjVolOCue9ashlS4EB5IECdSR2TItnAIiIwimx839\n\
LdUdRudafMu5T5Xma182OC0/u/xRlEm+tvKGGmfFcN0piqVl8OrSPBgIlb+1IKJE\n\
m/XriWr/Cq4h/JfB7NTsezVslgkBaoU=\n\
-----END CERTIFICATE-----\n\
"};
const int sizeof_cert_file = sizeof(cert_file);

bool client_thread_started = false;
bool socket_connected = false;
char disconnect_message[] = "Disconnected";
char connect_message[] = "Connected";

#define BUFFER_SIZE 2048
uint8_t reply_buffer[BUFFER_SIZE];
MqttClientContext mqtt_ctx;
MqttClientCallbacks mqtt_callbacks;
char password[50];
char device_id[50];
char device_topic[60];
char server_topic[60];
char username[60];
int sockfd = -1;
struct pollfd pollskcfd;
#define MAX_SEND_SIZE 1024

void connAckCallback(MqttClientContext *context, uint8_t connectAckFlags, uint8_t connectReturnCode)
{
    ESP_LOGE(TAG, "Connection Callback %d", connectReturnCode);
}
void publishCallback(MqttClientContext *context, const char *topic,
                     const uint8_t *message, uint16_t length, bool dup, MqttQosLevel qos, bool retain, uint16_t packetId)
{
    ESP_LOGE(TAG, "Incoming Data[%d]: %s", length, message);
}

void pubAckCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Publish ACK Callback %d", packetId);
}

void pubRecCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Publish REC Callback %d", packetId);
}

void pubRelCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Publish REL Callback %d", packetId);
}

void pubCompCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Publish COMP Callback %d", packetId);
}

void subAckCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Subscribe ACK Callback  %d", packetId);
}

void unsubAckCallback(MqttClientContext *context, uint16_t packetId)
{
    ESP_LOGE(TAG, "Unsubscribe ACK Callback %d", packetId);
}

void pingRespCallback(MqttClientContext *context)
{
    ESP_LOGE(TAG, "Ping RESP Callback ");
}

void client_thread(void *params)
{
    char buf[512];
    int ret, flags, len;
    uint16_t packet_id;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_context ssl;
    mbedtls_x509_crt cacert;
    mbedtls_ssl_config conf;
    mbedtls_net_context server_fd;
    mbedtls_x509_crl crl;
    mbedtls_ssl_init(&ssl);
    mbedtls_x509_crt_init(&cacert);
    ret = mbedtls_x509_crt_parse(&cacert, (const unsigned char *)cert_file, strlen(cert_file) + 1);
    if (ret != 0)
    {
        ESP_LOGE(TAG, "mbedtls_x509_crt_parse returned %d", ret);
        abort();
    }
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_ssl_config_init(&conf);
    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     NULL, 0)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ctr_drbg_seed returned %d", ret);
        abort();
    }

    ESP_LOGE(TAG, "Setting hostname for TLS session...");

    if ((ret = mbedtls_ssl_set_hostname(&ssl, TLS_SNI_HOSTNAME)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_set_hostname returned -0x%x", -ret);
        abort();
    }
    ESP_LOGE(TAG, "Setting up the SSL/TLS structure...");

    if ((ret = mbedtls_ssl_config_defaults(&conf,
                                           MBEDTLS_SSL_IS_CLIENT,
                                           MBEDTLS_SSL_TRANSPORT_STREAM,
                                           MBEDTLS_SSL_PRESET_DEFAULT)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_config_defaults returned %d", ret);
        goto exit;
    }
    mbedtls_ssl_conf_authmode(&conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(&conf, &cacert, NULL);
    mbedtls_ssl_conf_rng(&conf, mbedtls_ctr_drbg_random, &ctr_drbg);

    if ((ret = mbedtls_ssl_setup(&ssl, &conf)) != 0)
    {
        ESP_LOGE(TAG, "mbedtls_ssl_setup returned -0x%x", -ret);
        goto exit;
    }
    while (true)
    {

        mbedtls_net_init(&server_fd);

        ESP_LOGE(TAG, "Connecting to %s:%s...", TLS_SNI_HOSTNAME, SERVER_PORT);

        if ((ret = mbedtls_net_connect(&server_fd, TLS_SNI_HOSTNAME,
                                       SERVER_PORT, MBEDTLS_NET_PROTO_TCP)) != 0)
        {
            ESP_LOGE(TAG, "mbedtls_net_connect returned -%x", -ret);
            goto exit;
        }

        ESP_LOGE(TAG, "Connected.");

        mbedtls_ssl_set_bio(&ssl, &server_fd, mbedtls_net_send, mbedtls_net_recv, NULL);

        ESP_LOGE(TAG, "Performing the SSL/TLS handshake...");

        while ((ret = mbedtls_ssl_handshake(&ssl)) != 0)
        {
            if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE)
            {
                ESP_LOGE(TAG, "mbedtls_ssl_handshake returned -0x%x", -ret);
                goto exit;
            }
        }

        socket_connected = true;
        sockfd = server_fd.fd;
        ESP_LOGE(TAG, "Verifying peer X.509 certificate...");

        if ((flags = mbedtls_ssl_get_verify_result(&ssl)) != 0)
        {

            ESP_LOGE(TAG, "Failed to verify peer certificate!");
            bzero(buf, sizeof(buf));
            mbedtls_x509_crt_verify_info(buf, sizeof(buf), "  ! ", flags);
            ESP_LOGE(TAG, "verification info: %s", buf);
        }
        else
        {
            ESP_LOGE(TAG, "Certificate verified.");
        }

        ESP_LOGE(TAG, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(&ssl));

        if (ret == 0 && mqttClientInit(&mqtt_ctx) != 0)
        {
            ESP_LOGE(TAG, "MQTT Init Failed");
            ret = -1;
        }

        if (ret == 0)
        {
            mqttClientInitCallbacks(&mqtt_callbacks);
        }

        if (ret == 0 && mqttClientSetSSL(&mqtt_ctx, &ssl, &mbedtls_ssl_read, &mbedtls_ssl_write) != 0)
        {
            ESP_LOGE(TAG, "Failed to Set SSL");
            ret = -1;
        }

        if (ret == 0 && mqttClientSetSocket(&mqtt_ctx, sockfd) != 0)
        {
            ESP_LOGE(TAG, "Failed to Set Socket");
            ret = -1;
        }

        mqtt_callbacks.connAckCallback = &connAckCallback;
        mqtt_callbacks.publishCallback = &publishCallback;
        mqtt_callbacks.pubAckCallback = &pubAckCallback;
        mqtt_callbacks.pubRecCallback = &pubRecCallback;
        mqtt_callbacks.pubRelCallback = &pubRelCallback;
        mqtt_callbacks.pubCompCallback = &pubCompCallback;
        mqtt_callbacks.subAckCallback = &subAckCallback;
        mqtt_callbacks.unsubAckCallback = &unsubAckCallback;
        mqtt_callbacks.pingRespCallback = &pingRespCallback;

        if (ret == 0 && mqttClientRegisterCallbacks(&mqtt_ctx, &mqtt_callbacks) != 0)
        {
            ESP_LOGE(TAG, "Failed to Registered for Callback");
            ret = -1;
        }
        if (ret == 0 && mqttClientSetuptimeCallback(&mqtt_ctx, &esp_timer_get_time) != 0)
        {
            ESP_LOGE(TAG, "Failed to set ID value");
            ret = -1;
        }

        if (ret == 0 && mqttClientSetIdentifier(&mqtt_ctx, device_id) != 0)
        {
            ESP_LOGE(TAG, "Failed to set ID value");
            ret = -1;
        }

        if (ret == 0 && mqttClientSetAuthInfo(&mqtt_ctx, username, password) != 0)
        {
            ESP_LOGE(TAG, "Failed to Set Auth values");
            ret = -1;
        }

        if (ret == 0 && mqttClientSetWillMessage(&mqtt_ctx,
                                                 server_topic,
                                                 &disconnect_message,
                                                 sizeof(disconnect_message),
                                                 MQTT_QOS_LEVEL_2,
                                                 true) != 0)
        {
            ESP_LOGE(TAG, "Failed to Set Will message");
            ret = -1;
        }

        ESP_LOGE(TAG, "MQTT Auth Params %s, %s", mqtt_ctx.settings.password, mqtt_ctx.settings.username);
        ret = mqttClientConnect(&mqtt_ctx, true);
        if (ret != 0)
        {
            ESP_LOGE(TAG, "Failed to Connect MQTT %d", ret);
            ret = -1;
        }
        if (ret != 0)
        {
            socket_connected = false;
        }
        else
        {

            packet_id = 20;
            ret = mqttClientSubscribe(&mqtt_ctx, device_topic, MQTT_QOS_LEVEL_2, &packet_id);
            ret = mqttClientPublish(&mqtt_ctx, server_topic, &connect_message, sizeof(connect_message), MQTT_QOS_LEVEL_2, false, &packet_id);
        }
        pollskcfd.fd = sockfd;
        pollskcfd.events = POLLHUP;

        while (socket_connected == true)
        {
            int sock_connection = poll(&pollskcfd, 1, 10);
            if (sock_connection != 0)
            {
                socket_connected = false;
                goto exit;
            }
            else
            {
                ret = mqttClientTask(&mqtt_ctx, 10);
            }
        }

    exit:
        ESP_LOGE(TAG, "Socket Disconnected");
        mqttClientClose(&mqtt_ctx);

        mbedtls_ssl_session_reset(&ssl);
        mbedtls_net_free(&server_fd);

        printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

        for (int countdown = 10; countdown >= 0; countdown--)
        {
            ESP_LOGE(TAG, "%d...", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGE(TAG, "Starting again!");
    }
}
int publish_mqtt_message(char *message, int length)
{
    int ret = 0;
    uint16_t packet_id = 0;
    if (socket_connected)
    {
        return mqttClientPublish(&mqtt_ctx, server_topic, message, length, MQTT_QOS_LEVEL_2, false, &packet_id);
    }
    else
    {
        return -1;
    }
}

void start_client_thread(char *set_client_id, char *set_username, char *set_password, char *topic)
{
    if (!client_thread_started)
    {
        strcpy(device_id, set_client_id);
        strcpy(password, set_password);
        strcpy(username, set_username);
        sprintf(device_topic, "%s/a", topic);
        sprintf(server_topic, "%s/b", topic);
        client_thread_started = true;
        xTaskCreate(&client_thread, "mqtt_client_task", 8192, NULL, configMAX_PRIORITIES, NULL);
    }
}