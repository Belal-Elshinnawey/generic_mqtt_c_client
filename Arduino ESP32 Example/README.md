# Arduino ESP32 Example
Example uses the MQTT Generic client to connect to test.mosquitto.org:8885 using MBEDTLS

The example Subscribes to a topic and periodically sends a message after conncetion.

The same example can be used in ESP-IDF by changing the Setup function to main() and replacing the loop with a freertos task.
Don't forget to connect to wifi first of course. 
