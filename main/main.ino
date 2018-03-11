#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <String.h> 

typedef struct{
    char ssid[20];
    char password[50];
    int ward;
    int bed;
} Config;

/* Set these to your desired credentials. */
const char *ssid = "Mediot1";
const char *password = "randompassword";
const char *deviceID = "mediot1";

#define TRUE  1
#define FALSE 0


/*definitions
 * 
*/
void handleStatus();
void handleNotFound();
void handleSetup();
void handleFinishWizard();
void setupCredentials(const char *ssid, const char *password, int ward, int bed);
void sendError(char *message, int status);

int checkEEPROM();
void EEPROM_wifidata();
/*
 * END OF DEFINITIONS
 */



ESP8266WebServer server(8080);




void setup() {
	delay(1000);
	Serial.begin(115200);
	Serial.println();
	Serial.print("Configuring access point...");
	/* You can remove the password parameter if you want the AP to be open. */
	WiFi.softAP(ssid);

	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
  server.on("/api/status", handleStatus);
  server.on("/api/config", handleSetup);
  server.on("/api/finish/wizard", handleFinishWizard);
  server.onNotFound(handleNotFound);
	server.begin();
	Serial.println("HTTP server started");
}

void loop() {
	server.handleClient();
}


/*             Start of WIFI SERVER            
* /api/status for check
* /api/config for configurations
* /api/finish/wizard for end
*
*/

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

void handleSetup(){
  StaticJsonBuffer<300> JSONbuffer;   //Declaring static JSON buffer
  JsonObject& root = JSONbuffer.parseObject(server.arg("plain"));

  if(!root.success()){
    sendError("Parsing json failed", 500);
  }

  if(!root.containsKey("ssid")){
    sendError("SSID not found", 500);
  }

  if(!root.containsKey("ward")){
    sendError("Ward not found", 500);
  }

  if(!root.containsKey("bed")){
    sendError("Bed not found", 500);
  }

  const char *password = "";
  if(root.containsKey("password")){
    password = root["password"];
  }

  const char *ssid = root["ssid"];
  const int ward = root["ward"];
  const int bed = root["bed"];

  setupCredentials(ssid, password, ward, bed);
  
  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void handleFinishWizard(){
  server.send(200, "application/json", "{\"status\":\"done\"}");
  ESP.restart(); // restart to work with saved credentials
}

void setupCredentials(const char *ssid, const char *password, int ward, int bed){
  int addr = 0;
  int configSaved = 1;

  EEPROM.put(addr, configSaved);

  Config *myconfig = new Config();
  strcpy(myconfig->ssid, ssid);
  strcpy(myconfig->password, password);
  myconfig->ward = ward;
  myconfig->bed = bed;

  addr += sizeof(int);
  EEPROM.put(addr, myconfig);
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
