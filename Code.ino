#include "secrets.h"
#include "WiFi.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define AWS_IOT_PUBLISH_TOPIC   "esp32/pub"
#define AWS_IOT_SUBSCRIBE_TOPIC "esp32/sub"

const int trigPin = 2;
const int echoPin = 4;
// const int led1 = 5;
const int mq135Pin = 34;  

long duration;
float distance;
float co2Level;
float alcoholLevel;

WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);

void connectAWS() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  
  client.setCallback(messageHandler);
  Serial.println("Connecting to AWS IOT");
  while (!client.connect(THINGNAME))  {
    Serial.print(".");
    delay(100);
  }
  if (!client.connected())  {
    Serial.println("AWS IoT Timeout!");
    return;
  }
  client.subscribe(AWS_IOT_SUBSCRIBE_TOPIC);
  Serial.println("AWS IoT Connected!");
}

void publishMessage()  {
  StaticJsonDocument<200> doc;
  doc["distance"] = distance;
  doc["co2"] = co2Level;
  doc["alcohol"] = alcoholLevel;
  
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); 
  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}

void returnMessage(float newDistance) {
  StaticJsonDocument<200> retdoc;  
  retdoc["newdistance"] = newDistance;
  char jsonBuffer[512];
  serializeJson(retdoc, jsonBuffer); 
  client.publish(AWS_IOT_SUBSCRIBE_TOPIC, jsonBuffer);
}

void messageHandler(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.println(topic);

  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  Serial.print("Payload: ");
  Serial.println(message);


  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  
}


void setup() {
  Serial.begin(115200);
  connectAWS();
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  // pinMode(led1, OUTPUT);  
  pinMode(mq135Pin, INPUT);
}

void loop() {
  
  digitalWrite(trigPin, LOW);
  delay(2);
  digitalWrite(trigPin, HIGH);
  delay(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.0343 / 2;


  int mq135Value = analogRead(mq135Pin);
  co2Level = mq135Value * 0.1;       
  alcoholLevel = mq135Value * 0.05;  

  if (distance > 0) {
    Serial.print(F("Distance: "));
    Serial.print(distance);
    Serial.print(F(" cm, "));
    
    Serial.print(F("CO2 Level: "));
    Serial.print(co2Level);
    Serial.print(F(" ppm, "));
    
    Serial.print(F("Alcohol Level: "));
    Serial.print(alcoholLevel);
    Serial.println(F(" ppm"));
    
    publishMessage();
    
    // if (distance <= 10) {
    //   digitalWrite(led1, HIGH);
    //   Serial.println("----------->  ALERT");
    // }
    // else {
    //   digitalWrite(led1, LOW);
    // }
  }  
  else {
    Serial.println(F("Failed to read from sensor!"));
  }
  client.loop();
  delay(1000);
}
