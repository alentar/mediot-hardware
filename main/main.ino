#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include "FS.h"
#include <MQTTClient.h>

#define CLIENT_TOPIC "medIOT"
#define BROKER_USER "try"
#define BROKER_PASS "try"
#define HEARTPIN A0

unsigned long lastMillis = 0;

const char *AUTH_SELF = "";

char ST_SSID[20];
char ST_PASSWORD[20];
char ST_LINK[20];
char CLIENT_ID[30];
char TOPIC[100];
/* Set these to your desired credentials. */
const char *ssid = "Mediot1";
const char *password = "randompassword";
const char *deviceID = "mediot";


MQTTClient client;
WiFiClient wificlient;


/*definitions

*/
void handleStatus();
void handleNotFound();
bool handleSetup();
void handleFinishWizard();
void sendError(char *message, int status);
void configureSoftAP();

bool checkEEPROM();
bool loadConfig();
void EEPROM_wifidata();
bool stationMode();

void mqtt_connect();
boolean authorize();
/*
   END OF DEFINITIONS
*/



ESP8266WebServer server(8080);




void setup() {
  //starting eeprom and serial
  Serial.begin(115200);
  EEPROM.begin(512);  //in node EEPROM should init and EEPROM.commit to write.
  Serial.println("Mounting FS...");
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount file system");
    return;
  }
  Serial.setDebugOutput(true); //setting debug mode on

  if (!checkEEPROM()) {
    //starting the AP
    configureSoftAP();

    while (checkEEPROM() != true) {
      //we need to run the sever till the client writes the data to EEPROM
      server.handleClient();
      delay(3000);
    }
    //closing the AP

    WiFi.disconnect();
    WiFi.softAPdisconnect();
    EEPROM.end();
    delay(100);
  }
  else {
    Serial.println("HAS DATA ");
    if (!loadConfig()) {
      Serial.println("Failed to load config file");
    } else {
      Serial.println("Config loaded");
    }
    delay(1000);
    if (!stationMode()) {
      Serial.println("Failed to start Stationary mode");
    } else {
      Serial.println("Stationary mode started");
    }

    //starting MQTT
    char* broker = "192.168.8.100";


    while (!authorize()); //client sending post

    client.begin("192.168.8.100", wificlient);

  }

}

void loop() {

  int val = analogRead(HEARTPIN);
  client.loop();
  if (!client.connected()) {
    mqtt_connect();
  }

  if (millis() - lastMillis > 200) {
    lastMillis = millis();
    String topic = TOPIC;
    topic += "/type/bpm";
    client.publish(topic, (String)val);
    Serial.println(val);
  }
  delay(1000);

}

/*
   Device IDs
*/
boolean authorize() {

  String device_id = String(ESP.getChipId());
  String mac_id = String(WiFi.macAddress());          //Get the response payload



  const int httpPort = 3000;
  String host = "192.168.8.100";
  String data = "{\"mac\":\"" + mac_id + "\",\"chipId\":\"" + device_id + "\"}";
  String buffer;
  char json[1000];


  // String* host = ST_LINK;
  if (!wificlient.connect(host, httpPort)) {
    Serial.println("connection failed");
    delay(1000);
    return false;
  }

  Serial.print("Requesting POST: ");
  // Send request to the server:
  wificlient.println("POST /api/devices/auth/self HTTP/1.1");
  wificlient.print("Host: ");
  wificlient.println(host);
  wificlient.println("User-Agent: Arduino/1.0");
  wificlient.println("Connection: close");
  wificlient.println("Accept: */*");
  wificlient.println("Content-Type: application/json");
  wificlient.print("Content-Length: ");
  wificlient.println(data.length());
  wificlient.println();
  wificlient.print(data);

  delay(100); // Can be changed
  int counter = 0;

  char results[1500];
  char c ;
  int index = 0;
  while (wificlient.available())
  {
    c = wificlient.read();
    results[index] = c;
    index++;
  }
  results[index] = '\0';
  //Serial.println(results);
  int j = 0;
  for (int i = 433; i < index; i++) {
    json[j] = results[i];
    j++;
  }
  json[j] = '\0';


  Serial.println();
  Serial.println("closing connection");
  Serial.println("JSON Reply");
  Serial.println(json);

  const size_t bufferSize = JSON_OBJECT_SIZE(3) + 70;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(json);

  const char* state = root["state"];
  const char* operate = "operate";
  Serial.println(state);
  root.printTo(Serial);
  if (strcmp(state, operate) == 0) {
    Serial.println("HELLOOOOOOOOOOOOOO");
    const char* mqttTopic = root["mqttTopic"];
    const char* id = root["id"];
    Serial.println(mqttTopic);
    strcpy(TOPIC, mqttTopic);
    strcpy(CLIENT_ID, id);
    Serial.println(TOPIC);
    delay(100);
    return true;
  }


  delay(5000);
  return false;




}




/*
   End Device IDs
*/

/*
   START OF STATION MODE
*/
bool checkEEPROM() {

  int mem_loc = 0;
  int value;
  EEPROM.get(mem_loc, value);

  if (value == 1) {
    return true;
  } else {
    return false;
  }
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    Serial.println("Config file size is too large");
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> buf(new char[size]);


  configFile.readBytes(buf.get(), size);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  if (!json.success()) {
    Serial.println("Failed to parse config file");
    return false;
  }
  json.printTo(Serial);
  const char* ssid = json["ssid"];
  const char*  pass = json["password"];
  const char*  link = json["link"];
  delay(10);
  Serial.println(pass);
  strcpy(ST_PASSWORD, pass);
  strcpy(ST_LINK, link);
  strcpy(ST_SSID, ssid);
  delay(10);

  Serial.print("Loaded ssid: ");
  Serial.println(ST_SSID);
  Serial.print("Loaded password: ");
  Serial.println(ST_PASSWORD);
  Serial.print("Loaded link: ");
  Serial.println(ST_LINK);
  return true;
}

bool stationMode() {
  WiFi.disconnect();
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  if (WiFi.begin(ST_SSID, ST_PASSWORD)) {
    return true;
  } else {
    return false;
  }

}


/*
   END OF STATION MODE
*/






/*             Start of WIFI SERVER
  /api/status for check
  /api/config for configurations
  /api/finish/wizard for end

*/
void configureSoftAP() {
  //dc all
  WiFi.disconnect();
  WiFi.softAPdisconnect();

  Serial.println();
  Serial.print("Configuring access point...");
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.mode(WIFI_AP);
  delay(100);

  Serial.println(WiFi.softAP(ssid) ? "Ready" : "Failed!");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/api/status", handleStatus);
  server.on("/api/config", handleSetup);
  server.on("/api/finish", handleFinishWizard);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}


void handleStatus() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["status"] = "OK";
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  server.send(200, "application/json", JSONmessageBuffer);
}

void handleNotFound() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["message"] = "Not Found";
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  server.send(404, "application/json", JSONmessageBuffer);
}

bool handleSetup() {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& root = JSONbuffer.parseObject(server.arg("plain"));

  if (!root.success()) {
    sendError("Parsing json failed", 500);
  }

  if (!root.containsKey("ssid")) {
    sendError("SSID not found", 500);
  }

  if (!root.containsKey("password")) {
    sendError("Password not found", 500);
  }

  if (!root.containsKey("link")) {
    sendError("API link not found", 500);
  }

  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  root.printTo(configFile);
  server.send(200, "application/json", "{\"status\":\"ok\"}");
  delay(500);
  return true;
}

void handleFinishWizard() {

  int addr = 0;
  int configSaved = 1;
  server.send(200, "application/json", "{\"status\":\"done\"}");
  delay(500);
  EEPROM.put(addr, configSaved);
  delay(100);
  EEPROM.commit();

  ESP.restart(); // restart to work with saved credentials
}


void sendError(char *message, int status) {
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject();

  JSONencoder["message"] = message;
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  server.send(status, "application/json", JSONmessageBuffer);
}

/*             END OF WIFI SERVER               */


/*
   START MQTT CLIENT
*/

void mqtt_connect() {

  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("Connecting to Mqtt Server");
  Serial.println(CLIENT_ID);
  while (!client.connect(CLIENT_ID, CLIENT_ID, "none")) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println("\nConnected");

}


/*
   END OF MQTT CLIENT
*/


