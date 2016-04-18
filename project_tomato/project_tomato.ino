#include <I2CSoilMoistureSensor.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#include <ArduinoJson.h>

#include "config.h"

const int water_pin = 14; // D5 on LoLin board
const int cnt_sensor = 2;

/*
 * We are using the default pins for I2C:
 * * GPIO4 (D2): SDA
 * * GPIO5 (D1): SCL
 */
I2CSoilMoistureSensor sensor[cnt_sensor]; 

int sensor_addr[cnt_sensor];
int last_moisture[cnt_sensor];
int last_temp[cnt_sensor];
int last_light[cnt_sensor];

unsigned int last_poll = 0;
bool checked_light = 1;

ESP8266WebServer server(80);

void getAllData() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& moisture = root.createNestedArray("moisture");
  for (int i = 0; i < cnt_sensor; i++) {
    moisture.add(last_moisture[i]);
  }
  JsonArray& temp = root.createNestedArray("temperature");
  for (int i = 0; i < cnt_sensor; i++) {
    temp.add((float)last_temp[i] / (float)10, 2);
  }
  JsonArray& light = root.createNestedArray("light");
  for (int i = 0; i < cnt_sensor; i++) {
    light.add(last_light[i]);
  }
  String json_string;
  root.printTo(json_string);
  server.send(200, "application/json", json_string);
}

void getAllMoisture() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& moisture = root.createNestedArray("moisture");
  for (int i = 0; i < cnt_sensor; i++) {
    moisture.add(last_moisture[i]);
  }
  String json_string;
  root.printTo(json_string);
  server.send(200, "application/json", json_string);
}

void getAllTemperature() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& temp = root.createNestedArray("temperature");
  for (int i = 0; i < cnt_sensor; i++) {
    temp.add((float)last_temp[i] / (float)10, 2);
  }
  String json_string;
  root.printTo(json_string);
  server.send(200, "application/json", json_string);
}

void getAllLight() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& light = root.createNestedArray("light");
  for (int i = 0; i < cnt_sensor; i++) {
    light.add(last_light[i]);
  }
  String json_string;
  root.printTo(json_string);
  server.send(200, "application/json", json_string);
}


void handleRoot() {
  server.send(200, "text/html",
  "<html>"
    "<head>"
      "<title>Project Tomato Webinterface</title>"
    "</head>"
    "<body>"
      "<p>JSON-API</p>"
      "<ul>"
        "<li><a href='/getAllData'>Alle Daten abfragen</a></li>"
        "<li><a href='/getAllMoisture'>Alle Feuchtigkeitssensoren abfragen</a></li>"
        "<li><a href='/getAllTemperature'>Alle Temperatursensoren abfragen</a></li>"
        "<li><a href='/getAllLight'>Alle Lichtsensoren abfragen</a></li>"
      "</ul>"
    "</body>"
  "</html>");
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {
  pinMode(water_pin, OUTPUT);
  digitalWrite(water_pin, 0);

  Wire.begin();
  Serial.begin(9600);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);

  server.on("/getAllData", getAllData);
  server.on("/getAllMoisture", getAllMoisture);
  server.on("/getAllTemperature", getAllTemperature);
  server.on("/getAllLight", getAllLight);

  server.onNotFound(handleNotFound);

  // Set sensor adresses and initialize
  sensor[0].changeSensor(0x20);
  sensor[1].changeSensor(0x21);

  /*
  sensor1.begin(); // reset sensor
  sensor2.begin(); // reset sensor
  */
  delay(1000); // give some time to boot up
  Serial.println("Getting first sensor readings...");

  for (int i = 0; i < cnt_sensor; i++)
    sensor_addr[i] = sensor[i].getAddress();

  // Start first reading of sensors
  for (int i = 0; i < cnt_sensor; i++) {
    Serial.print("Moisture sensor ");
    Serial.print(i);
    Serial.print(": ");
    last_moisture[i] = sensor[i].getCapacitance();
    Serial.println(last_moisture[i]);
  }
  
  for (int i = 0; i < cnt_sensor; i++) {
    Serial.print("Temperature sensor ");
    Serial.print(i);
    Serial.print(": ");
    last_temp[i] = sensor[i].getTemperature();
    Serial.println(last_temp[i] / (float)10);
  }

  Serial.println("Start polling light sensors and wait for 3 seconds");
  // Start polling light sensors
  for (int i = 0; i < cnt_sensor; i++)
    sensor[i].startMeasureLight();
  delay(3000);
  for (int i = 0; i < cnt_sensor; i++) {
    last_light[i] = sensor[i].getLight();
    Serial.print("Light sensor ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(last_light[i]);
  }
  checked_light = 1;

  last_poll = millis();

  server.begin();
  Serial.println("HTTP server started");

}

void loop() {

  server.handleClient();

  unsigned int now = millis();

  if (last_poll + (1000 * poll_seconds) < now) {
    Serial.print(poll_seconds);
    Serial.println(" seconds are over, let's get data from the sensors");

    int moisture_sum = 0;

    for (int i = 0; i < cnt_sensor; i++) {
      Serial.print("Moisture sensor ");
      Serial.print(i);
      Serial.print(": ");
      last_moisture[i] = sensor[i].getCapacitance();
      moisture_sum += last_moisture[i];
      Serial.println(last_moisture[i]);
    }

    // Check if we need to water the soil
    if (moisture_sum / (float)cnt_sensor < moisture_threshold) {
        digitalWrite(water_pin, 1);
    } else {
        digitalWrite(water_pin, 0);
    }
  
    for (int i = 0; i < cnt_sensor; i++) {
      Serial.print("Temperature sensor ");
      Serial.print(i);
      Serial.print(": ");
      last_temp[i] = sensor[i].getTemperature();
      Serial.println(last_temp[i] / (float)10);
    }

    // Start polling light sensors
    Serial.println("Start polling light sensors and wait for 3 seconds");
    for (int i = 0; i < cnt_sensor; i++)
      sensor[i].startMeasureLight();

    checked_light = 0;
    last_poll = now;
  }

  if ((!checked_light) && (last_poll + 3000 < now)) {
    // Light is ready to read!
    for (int i = 0; i < cnt_sensor; i++) {
      last_light[i] = sensor[i].getLight();
      Serial.print("Light sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(last_light[i]);
    }
    
    checked_light = 1;
  }
}
