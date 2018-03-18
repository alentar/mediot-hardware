#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h>
#include "FS.h" 


char st_ssid[20];
char st_password[10];
char st_link[20];
/* Set these to your desired credentials. */
const char *ssid = "Mediot1";
const char *password = "randompassword";
const char *deviceID = "mediot";


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
    
   }
   
}

void loop() {
   Serial.print(" ssid: ");
  Serial.println(st_ssid);
  Serial.print(" password: ");
  Serial.println(st_password);
  Serial.print("Loaded link: ");
  Serial.println(st_link);
  Serial.println(WiFi.status());
	  delay(5000);
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

  const char* ssid = json["ssid"];
  const char*  pass= json["password"];
  const char*  link= json["link"];

  strcpy(st_ssid,ssid);
  strcpy(st_password,pass);
  strcpy(st_link,link);

  
  Serial.print("Loaded ssid: ");
  Serial.println(st_ssid);
  Serial.print("Loaded password: ");
  Serial.println(st_password);
  Serial.print("Loaded link: ");
  Serial.println(st_link);
  return true;
}

bool stationMode(){
  WiFi.disconnect();
  WiFi.softAPdisconnect();
  delay(100);
  WiFi.persistent(false);
  WiFi.mode(WIFI_OFF);
  WiFi.mode(WIFI_STA);
  if(WiFi.begin(st_ssid,st_password)){
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

