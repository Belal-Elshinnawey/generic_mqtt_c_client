#include <Arduino.h>
#include "mqtt_client_interface.h"
#include "WiFi.h"
char username[] = "rw";
char password[] = "readwrite";
char device_id[] = "mqttx_8fc8589x";

char topic[] = "belal_elshinnawey";
const char *networkName = "";
const char *networkPswd = "";
void connectToWiFi(const char *ssid, const char *pwd)
{
    Serial.println("Connecting to WiFi network: " + String(ssid));

    WiFi.begin(ssid, pwd);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}
char message[] = "Sup Sup";

void setup()
{
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    connectToWiFi(networkName, networkPswd);
    start_client_thread(device_id, username, password, topic);
}

void loop()
{
    publish_mqtt_message(message, strlen(message));

    vTaskDelay(1000);
}
