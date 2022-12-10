#include <SoftwareSerial.h>
#include "SNIPE.h"

#define POST_DATA  1
#define GET_DATA  2
#define POST_ACT 3

#define CODE  GET_DATA    /* Please define PING or PONG */

#if CODE == GET_DATA
#define TXpin 13
#define RXpin 15
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#else
#define TXpin 11 //WeMos 13 Arduino 11
#define RXpin 10 //WeMos 15 Arduino 10
#endif
#define ATSerial Serial

//16byte hex key
String lora_app_key = "11 22 33 44 55 66 77 88 99 aa bb cc dd ee ff 00";  

SoftwareSerial DebugSerial(RXpin,TXpin);
SNIPE SNIPE(ATSerial);
const char* ssid     = "****"; //your ssid
const char* password = "****"; //your pw
const char* host = "http://hiroshi.iptime.org:20000/post_data";
int pump_pin = 3;
int flamePin = 2;
int water_volumn = 750;
int buzzer = 6;
#if CODE == POST_DATA
int smokeA0 = A5;
#endif
int sensorThres = 900;

void setup() {
  #if CODE == POST_DATA
      pinMode(smokeA0, INPUT);
      //pinMode(TXpin, OUTPUT);
      Serial.begin(9600);
      
  #elif CODE == POST_ACT
      pinMode(buzzer, OUTPUT);
      pinMode(pump_pin,OUTPUT);
      digitalWrite(pump_pin,HIGH);
  #else
      WiFi.begin(ssid, password);
      while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
      }
  #endif
  ATSerial.begin(115200);

  // put your setup code here, to run once:
  while(ATSerial.read()>= 0) {}
  while(!ATSerial);

  DebugSerial.begin(115200);

  /* SNIPE LoRa Initialization */
  if (!SNIPE.lora_init()) {
    DebugSerial.println("SNIPE LoRa Initialization Fail!");
    while (1);
  }

  /* SNIPE LoRa Set Appkey */
  if (!SNIPE.lora_setAppKey(lora_app_key)) {
    DebugSerial.println("SNIPE LoRa app key value has not been changed");
  }
  
  /* SNIPE LoRa Set Frequency */
  if (!SNIPE.lora_setFreq(LORA_CH_1)) {
    DebugSerial.println("SNIPE LoRa Frequency value has not been changed");
  }

  /* SNIPE LoRa Set Spreading Factor */
  if (!SNIPE.lora_setSf(LORA_SF_7)) {
    DebugSerial.println("SNIPE LoRa Sf value has not been changed");
  }

  /* SNIPE LoRa Set Rx Timeout 
   * If you select LORA_SF_12, 
   * RX Timout use a value greater than 5000  
  */
  if (!SNIPE.lora_setRxtout(5000)) {
    DebugSerial.println("SNIPE LoRa Rx Timout value has not been changed");
  }  
    
  DebugSerial.println("SNIPE LoRa PingPong Test");
}
#if CODE == GET_DATA
void post_server(int fire=0, int polution=0) {
  if(WiFi.status() == WL_CONNECTED) {
  HTTPClient http;
  String jsondata = "";
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["fire"] = fire;
  root["polution"] = polution;

  root.printTo(jsondata);
  Serial.println(jsondata);
  
  http.begin(host);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsondata);
  Serial.println(httpResponseCode);
  }
  delay(60000);
}
#endif
void loop() {
  
#if CODE == POST_DATA //Smoke Detect Code : JunHo, Fire Detect Code : Ill-Yun
        int analogSensor = analogRead(smokeA0);
        int value = digitalRead(flamePin);
        DebugSerial.println(value);
        if(value == 0) {
          String send_msg = "POST_FIRE=1";
          if (SNIPE.lora_send(send_msg))
          {
            DebugSerial.println("Sensing Node : Send Success");
            String ver = SNIPE.lora_recv();
            DebugSerial.println(ver);
            if (ver == "ACK_DATA")
            {
              DebugSerial.println("Sensing Node : HandShake Success");           
            }
            else
            {
              DebugSerial.println("Sensing Node : recv fail");
              delay(500);
            }
          }
        }
        else if(analogSensor > sensorThres) {
          String send_msg = "POST_SMOKE="+String(analogSensor);
          if (SNIPE.lora_send(send_msg))
          {
            DebugSerial.println("Sensing Node : Send Success");
            String ver = SNIPE.lora_recv();
            DebugSerial.println(ver);
            if (ver == "ACK_DATA")
            {
              DebugSerial.println("Sensing Node : HandShake Success");           
            }
            else
            {
              DebugSerial.println("Sensing Node : recv fail");
              delay(500);
            }
          }
        }
        delay(1000);
       
#elif CODE == GET_DATA
        DebugSerial.println("TEST");
        String ver = SNIPE.lora_recv();
        int command_start = ver.indexOf("=");
        int ver_len = ver.length();
        String command = ver.substring(0, command_start);
        String value = ver.substring(command_start+1, ver_len);
        if (command == "POST_SMOKE" )
        {
          DebugSerial.println("Middle Node : Smoke recv success");
          if(SNIPE.lora_send("ACK_DATA"))
          {
            while(true) {
                SNIPE.lora_send("POST_BUZZER");
                String ver2 = SNIPE.lora_recv();
                if(ver2 = "ACK_ACT") {
                  DebugSerial.println(ver2);
                  break;
                }
                delay(1);
            }
            DebugSerial.println("Middle Node : Smoke_ACT send success");
            post_server(0, value.toInt());
          }
          else
          {
            DebugSerial.println("Middle Node : Smoke_ACT send fail");
            delay(500);
          }
        } else if(command == "POST_FIRE") {
          DebugSerial.println("Middle Node : Fire recv success");
          if(SNIPE.lora_send("ACK_DATA"))
          {
            while(true) {
                SNIPE.lora_send("POST_PUMP");
                String ver2 = SNIPE.lora_recv();
                if(ver2 = "ACK_ACT") {
                  DebugSerial.println(ver2);
                  break;
                }
                delay(1);
            }
            DebugSerial.println("Middle Node : Fire_ACT send success");
            post_server(1, 0);
          }
          else
          {
            DebugSerial.println("Middle Node : Fire_ACT send fail");
            delay(500);
          }
        }

#elif CODE == POST_ACT //Buzzer Code : Junho, Pump Code : ill-yun
        DebugSerial.println("ACT NODE TEST");
        String ver = SNIPE.lora_recv();
        DebugSerial.println(ver);
        if (ver == "POST_BUZZER" || ver.substring(0, 10) == "POST_SMOKE")
        {
          DebugSerial.println("ACT_NODE : SMOKE recv success");
          tone(buzzer, 1000, 5000);
          if(SNIPE.lora_send("ACK_ACT"))
          {
            DebugSerial.println("ACT_NODE : send success");
          }
          else
          {
            DebugSerial.println("send fail");
            delay(500);
          }
          delay(5000);
          noTone(buzzer);
        }
        else if(ver == "POST_PUMP" || ver.substring(0, 9) == "POST_FIRE") {
          DebugSerial.println("ACT_NODE : FIRE recv success");
          digitalWrite(pump_pin, LOW);
          if(SNIPE.lora_send("ACK_ACT"))
          {
            DebugSerial.println("send success");
          }
          else
          {
            DebugSerial.println("send fail");
            delay(500);
          }
          delay(8000);
          digitalWrite(pump_pin, HIGH);
       }
       delay(1);
#endif
}
