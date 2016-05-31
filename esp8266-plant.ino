
/*************************************************/
/* Includes                                      */
/*************************************************/
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "Client.h"
#include "myconfig.h"
#include "ThingSpeak.h"
#include <EEPROM.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>



#define SERIAL_SPEED 115200

// Create an WiFiClient object, here called "ethClient":
WiFiClient ethClient;
WiFiManager wifiManager;

//Create MQTT client object
PubSubClient client(mqtt_host, mqtt_port, ethClient);

char charBuffer[32];
int avValue = 0;
unsigned long oneSecond = 1000 * 1000;   
unsigned long sleepTime = 60 * 60 * oneSecond; // 1 hour  

// ------------------------
void setup() {
   // start serial port
   Serial.begin(SERIAL_SPEED);   

   if (debugOutput) Serial.println("ESP8266 starts...");

   sleep12Hours();
   
   pinMode(A0, INPUT);
   pinMode(2, OUTPUT);
   pinMode(4, OUTPUT);
   
   wifiManager.setTimeout(120);
   wifiManager.autoConnect();
   delay(100);
   do_update();
   
   digitalWrite(2, LOW);
   
   digitalWrite(4, HIGH);
   readValues();
   digitalWrite(4, LOW);
   
   send_Thingspeak();
   send_MQTT();
   
 
   if (debugOutput) { Serial.println("Deep sleep for 60 minutes..."); }
   ESP.deepSleep(60 * 60 * 1000 * 1000 - millis(),WAKE_RF_DISABLED); //minutes * 60 * 1000 * 1000 - durationInMillis
   delay(1000); //going into deep sleep tooks some time...
}

void loop() {
  //should never reached...
}

void readValues()
{
   int sensorvalue = 0;
   int sum = 0;
   int numberReadings = 1000;
   if (debugOutput) { Serial.println("start reading..."); }
   for(int i = 0; i < numberReadings; i++)
   { 
     sensorvalue = analogRead(A0);
     Serial.print(sensorvalue); Serial.print(" ");
     sum += sensorvalue;
   }
   digitalWrite(2, HIGH);
   avValue = sum/numberReadings;
   if (debugOutput) { 
    Serial.println("finished reading...");
    Serial.print("avValue: "); Serial.println(avValue); 
   }
   yield();

}

void send_MQTT()
{
   if (debugOutput) Serial.print("connecting to MQTT... ");
   while (!client.connected()) {
     if (client.connect(mqtt_id,mqtt_user,mqtt_pwd))  
      {
        if (debugOutput) Serial.println("connected");
        uint8_t MAC_array[6];
        char MAC_char[18];
        WiFi.macAddress(MAC_array);
        for (int i = 0; i < sizeof(MAC_array); ++i){
          sprintf(MAC_char,"%s%02x",MAC_char,MAC_array[i]);
        }
        String topic = String(mqtt_topic_prefix)+"/"+String(MAC_char);
        String strBuffer;
        strBuffer =  String(avValue);
        strBuffer.toCharArray(charBuffer,10);
        char topicBuffer[80];
        topic.toCharArray(topicBuffer,80);
        if (debugOutput) {
          Serial.print("Publishing to topic: ");Serial.print(topicBuffer);Serial.print(" value: ");Serial.println(charBuffer);
        }
        client.publish(topicBuffer,(uint8_t*)charBuffer,strlen(charBuffer),true);
      }
      else
      {
        if (debugOutput) Serial.println("MQTT is not connected... retrying");
        delay(200);
      }
   }
   // disconnect from MQTT
   client.disconnect();
   yield();
}

void send_Thingspeak(){
 ThingSpeak.begin(ethClient);
 ThingSpeak.setField(1,avValue);
 ThingSpeak.writeFields(thingspeak_channel, thingspeak_key);  
}

void do_update(){
  Serial.println("do update");
  t_httpUpdate_return ret = ESPhttpUpdate.update(update_server, 80, update_uri, firmware_version);
  switch(ret) {
    case HTTP_UPDATE_FAILED:
        Serial.println("[update] Update failed.");
        break;
    case HTTP_UPDATE_NO_UPDATES:
        Serial.print("[update] no Update needed current firmware version: "); Serial.println(firmware_version);
        break;
    case HTTP_UPDATE_OK:
        Serial.println("[update] Update ok."); // may not called we reboot the ESP
        break;
  }
}

/**
 * The deepSleep time is an uint32 of microseconds so a maximum of 4,294,967,295, about 71 minutes. 
 * So to publish a reading once per day needs to sleep for one hour 24 times and keep track of  
 * where its up to in RTC memory or EEPROM.
 * (RTC memory is broken, see https://github.com/esp8266/Arduino/issues/619) 
 * The WiFi radio is switched off for all but the 24th wakeup to save power
 */
void sleep12Hours() {
  byte rtcStore[2];
//  system_rtc_mem_read(64, rtcStore, 2);
  EEPROM.begin(4);
  rtcStore[0] = EEPROM.read(0);
  rtcStore[1] = EEPROM.read(1);
  printRtcStore(rtcStore);

  // 123 in [0] is a marker to detect first use   
  if (rtcStore[0] != 123) {
     rtcStore[0] = 123;
     rtcStore[1] = 0;
  } else {
     rtcStore[1] += 1;
  }
  if (rtcStore[1] > 12) {
     rtcStore[1] = 0;
  }
  printRtcStore(rtcStore);
//  system_rtc_mem_write(64, rtcStore, 2);
  EEPROM.write(0, rtcStore[0]);
  EEPROM.write(1, rtcStore[1]);
  EEPROM.end();
   
  if (rtcStore[1] == 0) {
    return; // a day is up, go do some work
  } 

  Serial.print("*** Up time: ");
  Serial.print(millis());
  Serial.print(", deep sleep for ");
  Serial.print(sleepTime/1000000);
  Serial.print(" seconds, with radio ");
  if (rtcStore[1] == 12) {
     Serial.println("on... ");
     ESP.deepSleep(1, WAKE_RF_DEFAULT);
  } else {
     Serial.println("off... ");
     ESP.deepSleep(sleepTime-millis(), WAKE_RF_DISABLED);
  }
}

void printRtcStore(byte* rtcStore) {
  Serial.print("rtcStore: ");
  Serial.print(rtcStore[0]);
  Serial.print(",");
  Serial.print(rtcStore[1]);
  Serial.println();
}

