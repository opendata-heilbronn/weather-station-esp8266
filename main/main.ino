#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>

#include <SoftwareSerial.h>
#include "DHT.h"

#include "config.h"

#define maxBuffer 80
WiFiClient client;
#define SERIALOUT 0


DHT dht(D2, DHT11);

SoftwareSerial mySerial(D8, 11); // RX, TX

int incomingByte = 0;


char inputBuffer[maxBuffer];   // For incoming serial data
int state1 = 0;


void setup() {
  pinMode(D0, OUTPUT);
  delay(250);
  digitalWrite(D0, LOW);


  if (SERIALOUT) {
    Serial.begin(38400);
    Serial.println("Start Platform IO");
  }
  mySerial.begin(19200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    toggleLed();
    if (SERIALOUT) {
      Serial.print(".");
    }
  }

  if (SERIALOUT) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  dht.begin();

}

void toggleLed() {
  if (state1 == 0) {
    state1 = 1;
    digitalWrite(D0, HIGH);
  } else {
    state1 = 0;
    digitalWrite(D0, LOW);

  }
}

void sendData(String data) {
  if (client.connect(server, 80)) { 
    //Serial.println(F("connected to server"));
    // Make a HTTP request:
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray& array = jsonBuffer.createArray();

    JsonObject& sensor = array.createNestedObject();

    sensor["type"] = "elv";
    sensor["value"] = data;

    String sensorJson = String("POST /rawSensor HTTP/1.0\r\nHost: "+hostName+"\r\nContent-Type: application/json\r\nConnection: close\r\n");

    int len = array.measureLength();
    sensorJson += "Content-Length: ";
    sensorJson += len;
    sensorJson += "\r\n\r\n";
    array.printTo(sensorJson);

    client.print(sensorJson);
    client.stop();
    if (SERIALOUT) {
      Serial.println(sensorJson);
      Serial.println("finished");
    }
  }
}

void sendTempSensor(float temp, float humidity) {
  if (client.connect(server, 80)) { 
    //Serial.println(F("connected to server"));
    // Make a HTTP request:
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray& array = jsonBuffer.createArray();

    JsonObject& sensor1 = array.createNestedObject();
    sensor1["sensorName"] = "cowo.inside3.temperature";
    sensor1["value"] = temp;

    JsonObject& sensor2 = array.createNestedObject();
    sensor2["sensorName"] = "cowo.inside3.humidity";
    sensor2["value"] = humidity;

    String sensorJson = String("POST /sensor HTTP/1.0\r\nHost: "+hostName+"\r\nContent-Type: application/json\r\nConnection: close\r\n");

    int len = array.measureLength();
    sensorJson += "Content-Length: ";
    sensorJson += len;
    sensorJson += "\r\n\r\n";
    array.printTo(sensorJson);

    client.print(sensorJson);
    client.stop();
    if (SERIALOUT) {
      Serial.println(sensorJson);
      Serial.println("finished");
    }
  }
}

unsigned long lastLocalSensorTime = 0;

void readAndSendLocalSensor() {
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    Serial.println("Temp: "+String(temperature)+ " Humidity: "+String(humidity));

    if (humidity > 0) {
      sendTempSensor(temperature, humidity);
    }
    lastLocalSensorTime = millis();

}

// the loop function runs over and over again forever
void loop() {
  toggleLed();

  long diff = lastLocalSensorTime - millis();
  if (abs(diff) > 180*1000) {
    Serial.println("Sending local sensor values... ");
    readAndSendLocalSensor();
  }

  if (SERIALOUT) {
    Serial.println(String(millis()) + "  -  "+String(abs(diff)));
  }

  while (mySerial.available() > 0) {
    String sensorData = "";

    byte bytesRead = mySerial.readBytes(inputBuffer, maxBuffer);
    for (int x = 0; x < bytesRead; x++) {
      if (inputBuffer[x] <= 0xf) {
        sensorData += "0";
      }
      sensorData += String(inputBuffer[x], HEX);
    }
    sendData(sensorData);
    if (SERIALOUT) {
      Serial.println(sensorData);
      Serial.println("----");
    }

  }
  delay(500);

}
