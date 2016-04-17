## Project Tomato

My project to make an automatic tomato waterer with a JSON-API and Wifi-Support.

### Used hardware

All links are examples, similar devices should work, too.

* ESP8266 with at least 3 GPIO-Ports available (in my case I'm using GPIO4, GPIO5 and GPIO) [Link](http://de.aliexpress.com/item/NodeMcu-Lua-WIFI-development-board-based-on-the-ESP8266-Internet-of-things/32338129505.html)
* 2 I2C capacitive soil moisture sensors [Link](https://www.tindie.com/products/miceuz/i2c-soil-moisture-sensor/)
* Electric Solenoid Valve [Link](http://eud.dx.com/product/electric-solenoid-valve-for-water-air-n-c-12v-dc-1-2-normally-closed-golden-white-844246864)
* 12V Relay
* Drip hose (with fitting pressure reducer)
* Water hose, adapters, ...

### Used software/libraries

* Arduino SDK 1.6.8 (or other current versions)
* ESP8266 hardware library (via [Arduino SDK board manager](http://arduino.esp8266.com/stable/package_esp8266com_index.json))
* ArduinoJson [Link](https://github.com/bblanchon/ArduinoJson)
* Modified version of I2C Soil Moisture Sensor library [Link](https://github.com/elzoido/I2CSoilMoistureSensor)
