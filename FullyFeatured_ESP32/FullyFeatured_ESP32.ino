
#include <WiFi.h>
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
}
#include <AsyncMqtt_Generic.h>
#include <Wire.h>
#include <WiFiManager.h>
#include "ArduinoJson.h"
#include <Adafruit_MLX90614.h>

#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerif9pt7b.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#include <MAX30100.h>
#include <MAX30100_BeatDetector.h>
#include <MAX30100_Filters.h>
#include <MAX30100_PulseOximeter.h>
#include <MAX30100_Registers.h>
#include <MAX30100_SpO2Calculator.h>
#define REPORTING_PERIOD_MS     6000

MAX30100 maxim;
PulseOximeter pox;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

// 'icons8-wi-fi-32', 16x16px
const unsigned char wifi16 [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0xf0, 0x3f, 0xfc, 0x7c, 0x3e, 0x71, 0x8e, 0x2f, 0xf0, 
  0x1f, 0xf8, 0x0c, 0x30, 0x01, 0x80, 0x03, 0xc0, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

const unsigned char nowifi [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x0d, 0xb0, 0x3d, 0xbc, 0x7d, 0xbe, 0x71, 0x8e, 
  0x25, 0xb4, 0x1d, 0xb8, 0x0c, 0x30, 0x00, 0x00, 0x01, 0x80, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00
};

#define WIFI_SSID "Redmi 9C"
#define WIFI_PASSWORD "012345678"

#define MQTT_HOST "broker.hivemq.com"
#define MQTT_PORT 1883

uint32_t tsLastReport = 0;
int  h ,flag = 0;
float t , temp =36.5;
long lastReconnectAttempt = 0;
#define topic1 "nguyenvanhuan-B18DCDT088"

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void onBeatDetected(){
    Serial.println("Beat!");
  //  pox.update();
}

void connectToWifi() {
//  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
//  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
//    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
//        Serial.println("WiFi connected");
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
//        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
//  Serial.println("Connected to MQTT.");
//  Serial.print("Session present: ");
//  Serial.println(sessionPresent);
  mqttClient.subscribe("lamp-topic" ,0);
  mqttClient.subscribe("fan-topic" , 0);
  mqttClient.subscribe("dehumi-topic", 0);

}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
//  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
     String messageTemp;
     for (int i = 0; i < len; i++) {
        messageTemp += (char)payload[i];
     }
     // Check if the MQTT message was received on topic 
     if (strcmp(topic, "lamp-topic") == 0) {
       // If the relay is on turn it off (and vice-versa)
        if (messageTemp == "1") digitalWrite(4, HIGH);
        else   digitalWrite(4, LOW);
     }
     
     if (strcmp(topic, "fan-topic") == 0) {
     // If the relay is on turn it off (and vice-versa)
        if (messageTemp == "1") digitalWrite(0, HIGH);
        else    digitalWrite(0, LOW);
     }
     
     if (strcmp(topic, "dehumi-topic") == 0) {
     // If the relay is on turn it off (and vice-versa)
        if (messageTemp == "1") digitalWrite(2, HIGH);
        else    digitalWrite(2, LOW);
     }
}

//void onMqttPublish(uint16_t packetId) {
//  Serial.println("Publish acknowledged.");
//  Serial.print("  packetId: ");
//  Serial.println(packetId);
//}


void setup() {
    Serial.begin(115200);
    
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
    wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

    WiFi.onEvent(WiFiEvent);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
//    mqttClient.onPublish(onMqttPublish);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);

    connectToWifi();
    pinMode(4, OUTPUT); // bedroomLight
    pinMode(0, OUTPUT); // bedroomAirConditioner
    pinMode(2, OUTPUT); // airVent

    mqttClient.subscribe("lamp-topic" , 0);
    mqttClient.subscribe("fan-topic" , 0);
    mqttClient.subscribe("dehumi-topic" , 0);
  
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {  
      Serial.println("SSD1306 allocation failed");
      for(;;);
    }
    display.setFont(&FreeSerif9pt7b);
    display.clearDisplay();
    display.setTextSize(1);             
    display.setTextColor(WHITE);        
    display.setCursor(22,15);             
    display.println("Welcom to");
    display.setCursor(18,40);             
    display.println("Track-Covid");
    display.display();
    display.setTextSize(1);
    delay(2000); 
    
    if (!mlx.begin()) {
       Serial.println("Error connecting to MLX sensor. Check wiring.");
       while (1);
    };
    
    Serial.print("Initializing pulse oximeter..");
    if (!pox.begin()) {
        Serial.println("FAILED");
        for(;;);
    } else {
        Serial.println("SUCCESS");
        pox.setOnBeatDetectedCallback(onBeatDetected);
    }
  //  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    
}

void loop() {
    pox.update();
    t = pox.getHeartRate();
    h = pox.getSpO2();
    if (millis() - tsLastReport > REPORTING_PERIOD_MS){

      if (WiFi.status() == WL_CONNECTED) flag = 1;
      else flag = 0;
//       Serial.printf("Connection status: %d\n", WiFi.status());
       
       if (int(pox.getHeartRate()) > 40) t = pox.getHeartRate();
       else t = 0;
       if (pox.getSpO2() > 90 && pox.getSpO2() < 100 ) h = pox.getSpO2();
       else h = 0;
       if(mlx.readObjectTempC() > 36) temp = mlx.readObjectTempC();
       Serial.print("Heart rate:");
       Serial.print(pox.getHeartRate());
       Serial.print("bpm / SpO2:");
       Serial.print(pox.getSpO2());
       Serial.println("%");
       Serial.print("Temp:");
       Serial.print(mlx.readObjectTempC());
       Serial.println("*C");
       display.clearDisplay(); 

       if (flag == 1) display.drawBitmap(112, 0, wifi16, 16, 16, WHITE);
       else display.drawBitmap(110, 0, nowifi, 16, 16, WHITE);
       display.setCursor(0,12); 
       display.print("Spo2 :");
       display.setCursor(57,12);
       display.print(h);
       display.println(" %");

       display.setCursor(0,35); 
       display.print("Heart:");
       display.setCursor(57,35);
       display.print(t,0); 
       display.println(" bpm");
       display.setCursor(0,59);
       display.print("Temp: ");
       display.setCursor(57,59);
       display.print(temp,1);
       display.println(" *C");
       display.display();
    
       int bar = int(temp * 100); // bar now = 12345
       
       StaticJsonDocument<100> doc;
       
        doc["Heart"] = int(t);
        doc["Spo2"] = h;
        doc["BodyTemp"] = bar* 0.01;
        doc["Eco2"] = 400;
       
       
       char buffer[256];
       serializeJson(doc, buffer);
       mqttClient.publish(topic1, 1, true, String(buffer).c_str());
     //  Serial.println(buffer);

       tsLastReport = millis();
   }
   maxim.resetFifo();
}
