/*
 *Netmedias
 *
 *  Created on: 24.05.2015
 *  
 */
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <Hash.h>

const char* ssid     = "SBG6700AC-B374F";
const char* password = "6ddb0ad169";
char host[] = "iotesp8266nodemcu.herokuapp.com";

int port = 80;
char path[] = "/ws"; 
ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
const int relayPin = 16;
const int readyPin = 05;
DynamicJsonBuffer jsonBuffer;
String currState;
int pingCount = 0;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
       Serial.println("Disconnected! ");
       digitalWrite(readyPin, LOW);
       Serial.println("Connecting...");
           webSocket.begin(host, port, path);
           webSocket.onEvent(webSocketEvent);
        break;
        
    case WStype_CONNECTED:{
        Serial.println("Connected! ");
        digitalWrite(readyPin, HIGH);
        webSocket.sendTXT("Connected");
        }
        break;
        
    case WStype_TEXT:
        Serial.println("Got data");
        processWebScoketRequest((char*)payload);
        break;
        
    case WStype_BIN:
        hexdump(payload, length);
        Serial.print("Got bin");
        // send data to server
        webSocket.sendBIN(payload, length);
        break;
   }
}

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(true);
    
    pinMode(relayPin, OUTPUT);
    pinMode(readyPin, OUTPUT);
    digitalWrite(readyPin, LOW);
    digitalWrite(relayPin, LOW);
    
    for(uint8_t t = 4; t > 0; t--) {
        delay(1000);
    }
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    
    WiFiMulti.addAP(ssid, password);

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
        delay(100);
    }
    Serial.println("Connected to wi-fi");
    webSocket.begin(host, port, path);
    webSocket.onEvent(webSocketEvent);
}

void loop() {
    webSocket.loop();
  //If you make change to delay mak sure adjust the ping
    delay(2000);
  // make sure after every 40 seconds send a ping to Heroku
  //so it does not terminate the websocket connection
  //This is to keep the conncetion alive between ESP and Heroku
  if (pingCount > 20) {
    pingCount = 0;
    webSocket.sendTXT("\"heartbeat\":\"keepalive\"");
  }
  else {
    pingCount += 1;
  }
}

void processWebScoketRequest(String data){
  JsonObject& root = jsonBuffer.parseObject(data);
  Serial.println("Recieved data! : ");
  String device = (const char*)root["device"];
  String location = (const char*)root["location"];
  String state = (const char*)root["state"];
  String query = (const char*)root["query"];
  String message="";

  Serial.println(data);
  if(query == "cmd"){ //if query check state
    Serial.println("Recieved command!");
    if(state=="on"){
      digitalWrite(relayPin, HIGH);
      message = "{\"state\":\"ON\"}";
      currState = "ON";
    }else if(state=="off"){
      digitalWrite(relayPin, LOW);
      message = "{\"state\":\"OFF\"}";
      currState = "OFF";
    }
  }else if(query == "?"){ //if command then execute   
    Serial.println("Recieved query!");
    int state = digitalRead(relayPin);
       if(currState=="ON"){
            message = "{\"state\":\"ON\"}";
          }else if(currState=="off"){
            message = "{\"state\":\"OFF\"}";
          }
  }else{//can not recognized the command
    Serial.println("Command is not recognized!");
  }
  Serial.print("Sending response back");
  Serial.println(message);
  // send message to server
  webSocket.sendTXT(message);
  if(query == "cmd" || query == "?"){webSocket.sendTXT(message);}
}