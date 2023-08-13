# generic_mqtt_c_client
MQTT C client using POSIX Socket API and Posix thread API

The purpose of this library is to provide a generic MQTT client for embedded devices independant from whatever TLS provided tools or Network API.


To use the library your framework has to offer 2 things:
1- POSIX Sockets API for POLLIN & POLLHUP, read/write operations are abstracted (See Example)
2- POSIX Thread API for mutex_lock and unlock.

The library was tested using NRF_Connect_SDK with WOLFSSL, ESP-IDF and Arduino with MBEDTLS and both show acceptable performance.

In order to implement the library in you project follow these steps:

  1- Make sure your framework has posix API enabled.
 
  2- Setup an SSL/TLS library if needed
 
  3- Create an interface file similar to mqtt_client_interface.c in the included example
 
  4- In the interface, create a function that sets up a thread to init the MQTT Connections
 
  5- In the Connection thread, establish a TCP connection and if needed the TLS handshake
 
  6- Create Callback functions for publish, subscribe, ping resp, ack, etc... the callbacks are blocking so it is recommended
      to pass events to seprate workQ
     
  7- Setup the MQTT library by calling mqttClientInit and then pass the callbacks. The process after the TLS handshake should be identical to the example.
 

You should be able to use the librery after calling mqttClientConnect. Maintain the Connection by periodicall calling mqttClientTask in your application from the same thread.


