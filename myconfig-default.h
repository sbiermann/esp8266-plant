/*************************************************/
/* Debugging                                     */
/*************************************************/
const bool debugOutput = true;  // set to true for serial OUTPUT

/*************************************************/
/* Update settings                               */
/*************************************************/ 
const char* firmware_version = "espplant_0.0.1";
const char* update_server = "localhost";
const char* update_uri = "/esp/update/arduino.php";

/*************************************************/
/* Thingspeak data                               */
/*************************************************/
const char* thingspeak_key = "secret";
const int thingspeak_channel = 4711;

/*************************************************/
/* MQTTCloud data                               */
/*************************************************/
const char* mqtt_host = "m21.cloudmqtt.com";
const char* mqtt_user = "user";
const char* mqtt_pwd = "secret";
const char* mqtt_id = "ESP8266.1";
const char* mqtt_topic_prefix = "sensor";
const int mqtt_port = 15377;

