
# MedIOT-hardware

## Intro

**MedIOT** project uses the NodeMCU as main embedded system. It is selected nodeMCU as the main platform because it contains an integrated ESP8266MOD wifi module with Arduino nano board. A digital temperature sensor DS18B20 sensor and Pulse sensor is using as inputs to the nodemodule.

## Before get started

 1. Install [Arduino SDK](https://www.arduino.cc/en/Main/Software) and [ESP8266 Board Manager](https://github.com/esp8266/Arduino#installing-with-boards-manager)
 
 3. Install following Arduino libraries by `Sketch>Library>Library Manager` ;
	 - Arduino JSON- (by Benoit Blanchan) [Link](https://arduinojson.org/?utm_source=meta&utm_medium=library.properties)
	 - Arduino Mqtt (by Oleg Kevolenko) - [Link](https://github.com/monstrenyatko/ArduinoMqtt)
	 
 4.  Set the sketch option from `Tools` menu as follows ![screenshot_4](https://user-images.githubusercontent.com/18147085/37981376-334bc38c-320b-11e8-8374-dbb502e9d963.jpg)

## Board Configuration
 In this, we are using two sensors, Heart Pulse sensor and Temperature sensor. Pulse sensor is given to A0 input, and Temperature sensor DS18B20 given to GPIO input

For `ESP.Restart()` nodemcu checking the pins for booting. Here we are **boot from the sketch**  So according to the following table  **GPIO0** and **GPIO1** should connected to **3V3 Vcc** and **GPIO15** should connected to **GND** with resistors . 
 

> if the wdt reset is triggert the board reboots, when it **reboots it
> is checking the state of GPIO0 and GPIO15 and GPIO2**

|GPIO15|GPIO0|GPIO2|Mode|
|--|--|--|--|
|0V|0V|	3.3V|Uart Bootloader||
|0V|3.3V|3.3V|**Boot sketch**||
|3.3V|x|x|SDIO mode (not used for Arduino)|

> **you need to make sure that the pins in the right state when a wtd happens or you call ESP.restart() better you add some delays to
> prevent the wtd from triggering.**

**`Important :
All the above connetions should be made with a 10K resistors`**

## Run the code

 1. Compile and Upload the code to the Nodemcu
 2. Open SerialMonitor
 3. Restart the Nodemcu 
 4. SoftAP WiFi is set on the relevent ip address shown in the serial monitor. *default WiFi SSID*-> `Mediot1`

##  API End Points
API end points of the Sever as follows;

 #### Server default Address: `192.168.4.1:8080`
-----
### Show Server Status

Get the status of the server.

**URL** : `/api/status/`
**Method** : `GET`
**Auth required** : NO
**Permissions required** : None

 Success Response

**Code** : `200 OK`
```json
{
    "status": "ok"
}
```

---
### Set Configurations

Setting the WiFi credentials and API end point of the mqtt server

**URL** : `/api/config/`
**Method** : `POST`
**Auth required** : --
**Permissions required** : --
**Data constraints**

Provide wifi details and api end point.

```json
{
	"ssid":"YOURWIFISSID",
	"password":"YOURWIFIPASSWORD",
	"link":"MQTT_SERVER"
}
```

### Success Response

**Code** : `200 OK`
```json
{
    "status": "ok"
}
```
---
### Save Configuration

Saving the given wifi credentials and end point

**URL** : `/api/finish`
**Method** : `GET`
**Auth required** : NO
**Permissions required** : None

 Success Response

**Code** : `200 OK`
```json
{
    "status": "ok"
}
```
