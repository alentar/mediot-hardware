#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include "FS.h" 
#include <MQTTClient.h>

#define CLIENT_TOPIC "medIOT"
#define CLIENT_ID "ESP-TEST"
#define BROKER_USER "try"
#define BROKER_PASS "try"
#define HEARTPIN A0

unsigned long lastMillis=0;

char ST_SSID[20];
char ST_PASSWORD[20];
char ST_LINK[20];
/* Set these to your desired credentials. */
const char *ssid = "Mediot1";
const char *password = "randompassword";
const char *deviceID = "mediot";

MQTTClient client;
WiFiClient wificlient;


/*definitions
 * 
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
/*
 * END OF DEFINITIONS
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
 
   if(!checkEEPROM()){
    //starting the AP
    configureSoftAP();

    while(checkEEPROM()!=true){
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
   else{
     Serial.println("HAS DATA ");
    if(!loadConfig()){
      Serial.println("Failed to load config file");
    }else{
      Serial.println("Config loaded");
    }
    delay(1000);
    if(!stationMode()){
      Serial.println("Failed to start Stationary mode");
    }else{
      Serial.println("Stationary mode started");
    }

    //starting MQTT
    client.begin(ST_LINK,wificlient);
    
   }
   
}

void loop() {
//   Serial.print(" ssid: ");
//  Serial.println(st_ssid);
//  Serial.print(" password: ");
//  Serial.println(st_password);
//  Serial.print("Loaded link: ");
//  Serial.println(ST_LINK);
//  Serial.println(WiFi.status());
  int val=analogRead(HEARTPIN);
    client.loop();
     if(!client.connected()) {
     mqtt_connect();
   }
  
   if(millis() - lastMillis > 200) {
     lastMillis = millis();
     client.publish(CLIENT_TOPIC, (String)val);
      Serial.println(val);
  }
 delay(10);
  
}


/*
 * START OF STATION MODE
 */
bool checkEEPROM(){
  
  int mem_loc=0;
  int value;
  EEPROM.get(mem_loc,value);

  if(value==1){
    return true;
  }else{
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
  const char*  pass= json["password"];
  const char*  link= json["link"];
  delay(10);
  Serial.println(pass);
  strcpy(ST_PASSWORD,pass);
  strcpy(ST_LINK,link);
  strcpy(ST_SSID,ssid);
  delay(10);
  
  Serial.print("Loaded ssid: ");
  Serial.println(ST_SSID);
  Serial.print("Loaded password: ");
  Serial.println(ST_PASSWORD);
  Serial.print("Loaded link: ");
  Serial.println(ST_LINK);
  return true;
}

bool stationMode(){
  WiFi.disconnect();
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  if(WiFi.begin(ST_SSID,ST_PASSWORD)){
    return true;
  }else{
    return false;
  }
  
}


/*
 * END OF STATION MODE
 */






/*             Start of WIFI SERVER            
* /api/status for check
* /api/config for configurations
* /api/finish/wizard for end
*
*/
void configureSoftAP(){
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


void handleStatus(){
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject(); 

  JSONencoder["status"] = "OK";
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  server.send(200, "application/json", JSONmessageBuffer);
}

void handleNotFound(){
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject(); 

  JSONencoder["message"] = "Not Found";
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  server.send(404, "application/json", JSONmessageBuffer);
}

bool handleSetup(){
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& root = JSONbuffer.parseObject(server.arg("plain"));

  if(!root.success()){
    sendError("Parsing json failed", 500);
  }

  if(!root.containsKey("ssid")){
    sendError("SSID not found", 500);
  }

  if(!root.containsKey("password")){
    sendError("Password not found", 500);
  }

  if(!root.containsKey("link")){
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

void handleFinishWizard(){
  
  int addr = 0;
  int configSaved = 1;
  server.send(200, "application/json", "{\"status\":\"done\"}");
  delay(500);
  EEPROM.put(addr, configSaved);
  delay(100);
  EEPROM.commit();
 
  ESP.restart(); // restart to work with saved credentials
}


void sendError(char *message, int status){
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& JSONencoder = JSONbuffer.createObject(); 

  JSONencoder["message"] = message;
  char JSONmessageBuffer[300];
  JSONencoder.prettyPrintTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  server.send(status, "application/json", JSONmessageBuffer);
}

/*             END OF WIFI SERVER               */


/*
 * START MQTT CLIENT
 */

void mqtt_connect(){

  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }
  
  Serial.println("Connecting to Mqtt Server");
   while (!client.connect("nodemcu","try","try")) {
   Serial.print(".");
   delay(1000);
   }
  Serial.println("\nConnected");
  client.subscribe("nodemcu");
}


 /*
  * END OF MQTT CLIENT
  */

