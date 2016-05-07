#include <I2CSoilMoistureSensor.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "Adafruit_IO_Client.h"

#include <ArduinoJson.h>

#include "config.h"

#define SERIALOUT 1

const int water_pin = 14; // D5 on LoLin board
const int cnt_sensor = 2;

/*
 * We are using the default pins for I2C:
 * * GPIO4 (D2): SDA
 * * GPIO5 (D1): SCL
 */

WiFiClient client;
Adafruit_IO_Client aio = Adafruit_IO_Client(client, AIO_KEY);

I2CSoilMoistureSensor sensor[cnt_sensor]; 

int sensor_addr[cnt_sensor];

int last_moisture[cnt_sensor];
int last_temp[cnt_sensor];
int last_light[cnt_sensor];

Adafruit_IO_Feed feed_moisture = aio.getFeed("moisture");
Adafruit_IO_Feed feed_temp = aio.getFeed("temperature");
Adafruit_IO_Feed feed_light = aio.getFeed("light");
Adafruit_IO_Feed feed_threshold_min = aio.getFeed("water-threshold-min");
Adafruit_IO_Feed feed_threshold_max = aio.getFeed("water-threshold-max");
Adafruit_IO_Feed feed_watering_status = aio.getFeed("current-watering-status");

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

  if (SERIALOUT)
    Serial.begin(9600);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (SERIALOUT)
      Serial.print(".");
  }

  if (SERIALOUT) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  server.on("/", handleRoot);

  server.on("/getAllData", getAllData);
  server.on("/getAllMoisture", getAllMoisture);
  server.on("/getAllTemperature", getAllTemperature);
  server.on("/getAllLight", getAllLight);

  server.onNotFound(handleNotFound);

  aio.begin();

  // Set sensor adresses and initialize
  sensor[0].changeSensor(0x20);
  sensor[1].changeSensor(0x21);

  /*
  sensor1.begin(); // reset sensor
  sensor2.begin(); // reset sensor
  */
  delay(1000); // give some time to boot up
  if (SERIALOUT)
    Serial.println("Getting first sensor readings...");

  for (int i = 0; i < cnt_sensor; i++)
    sensor_addr[i] = sensor[i].getAddress();

  // Start first reading of sensors
  int sum_moisture = 0;
  int sum_temp = 0;
  int sum_light = 0;
  
  for (int i = 0; i < cnt_sensor; i++) {
    last_moisture[i] = sensor[i].getCapacitance();
    sum_moisture += last_moisture[i];
    if (SERIALOUT) {
      Serial.print("Moisture sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(last_moisture[i]);
    }
  }
  feed_moisture.send(sum_moisture / cnt_sensor);  
  
  for (int i = 0; i < cnt_sensor; i++) {
    last_temp[i] = sensor[i].getTemperature();
    sum_temp += last_temp[i];
    if (SERIALOUT) {
      Serial.print("Temperature sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(last_temp[i] / (float)10);
    }
  }
  feed_temp.send(sum_temp / (cnt_sensor * 10.0));

  if (SERIALOUT)
    Serial.println("Start polling light sensors and wait for 3 seconds");
  // Start polling light sensors
  for (int i = 0; i < cnt_sensor; i++)
    sensor[i].startMeasureLight();
  delay(3000);
  for (int i = 0; i < cnt_sensor; i++) {
    last_light[i] = sensor[i].getLight();
    sum_light += last_light[i];
    if (SERIALOUT) {
      Serial.print("Light sensor ");
      Serial.print(i);
      Serial.print(": ");
      Serial.println(last_light[i]);
    }
  }
  feed_light.send(sum_light / cnt_sensor);
  
  checked_light = 1;

  last_poll = millis();

  server.begin();
  if (SERIALOUT)
    Serial.println("HTTP server started");

}

void loop() {

  server.handleClient();

  unsigned int now = millis();

  if (last_poll + (1000 * poll_seconds) < now) {
    if (SERIALOUT) {
      Serial.print(poll_seconds);
      Serial.println(" seconds are over, let's get data from the sensors");
    }

    int sum_moisture = 0;
    int sum_temp = 0;

    FeedData min = feed_threshold_min.receive();
    if (min.isValid()) {
      min.intValue(&water_threshold_min);
    }
    FeedData max = feed_threshold_max.receive();
    if (max.isValid()) {
      max.intValue(&water_threshold_max);
    }
    
    for (int i = 0; i < cnt_sensor; i++) {
      last_moisture[i] = sensor[i].getCapacitance();
      sum_moisture += last_moisture[i];
      if (SERIALOUT) {
        Serial.print("Moisture sensor ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(last_moisture[i]);
      }
    }
    feed_moisture.send(sum_moisture / cnt_sensor);  


    // Check if we need to water the soil
    if (sum_moisture / (float)cnt_sensor < water_threshold_min) {
        digitalWrite(water_pin, 1);
        feed_watering_status.send(1);
    } else if (sum_moisture / (float)cnt_sensor > water_threshold_max) {
        digitalWrite(water_pin, 0);
        feed_watering_status.send(0);
    }

    for (int i = 0; i < cnt_sensor; i++) {
      last_temp[i] = sensor[i].getTemperature();
      sum_temp += last_temp[i];
      if (SERIALOUT) {
        Serial.print("Temperature sensor ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(last_temp[i] / (float)10);
      }
    }
    feed_temp.send(sum_temp / (cnt_sensor * 10.0));

    // Start polling light sensors
    if (checked_light) {
      if (SERIALOUT)
        Serial.println("Start polling light sensors and wait for 3 seconds");

      for (int i = 0; i < cnt_sensor; i++)
        sensor[i].startMeasureLight();

      checked_light = 0;
    }
    last_poll = now;
  }

  if ((!checked_light) && (last_poll + 3000 < now)) {
    // Light is ready to read!

    int sum_light = 0;

    for (int i = 0; i < cnt_sensor; i++) {
      last_light[i] = sensor[i].getLight();
      sum_light += last_light[i];
      if (SERIALOUT) {
        Serial.print("Light sensor ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(last_light[i]);
      }
    }
    feed_light.send(sum_light / cnt_sensor);
        
    checked_light = 1;
  }
}
