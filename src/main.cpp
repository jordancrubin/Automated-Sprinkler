/*
  Rubin Projects Boot Framework and ASYNC web Based Wifi configuration Framework 
  Standardized basic framework for my projects that require no special code nor
  any extra libraries beyond what I regularly use in my projects to put an ESP32
  on the network with an interactive Web based GUI. 
  https://www.youtube.com/c/jordanrubin6502
  2023-2026 Jordan Rubin.
*/

#define FACDEFDELAY 5000
#define FACDEFPIN 0
#define LCDCOLS 4
#define LCDI2C 0x27
#define LCDROWS 20
#define PF575I2C 0x20
#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 4
#define ROTARY_ENCODER_BUTTON_PIN 33
#define ROTARY_ENCODER_STEPS 4
#define ROTARY_ENCODER_VCC_PIN -1
#define WEBSERVPORT 80
#include "AiEsp32RotaryEncoder.h"
#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "mbedtls/md.h"
#include <NTPClient.h>
#include <Sprinkler.h>
#include <time.h>
#include <WiFiAP.h>
#include <WiFi.h>
#include <HTTPClient.h>

const char* deviceName = "sprinkler32";  //Mdns name sprinkler32.local
double lastTimepoll = -60000;
const char* ntpServer = "pool.ntp.org";
const char* ssid = "sprinklernet";    //SSID of the netconfig Access point
struct tm timeinfo;
bool displayLock;
int displayprogram = 0;
int lastEncval;
int menulevel   = 0;
int selectlevel = 0;
int updatepullstatsflag = -1;
unsigned long powerDebounceTime = 0;
int programMenu = -1;
bool setManualFlag = false;
bool setCancelFlag = false;
bool zoneChangeFlag = false;
bool shouldRestart = false;
unsigned long restartTime = 0;
bool isTestingWifi = false;
unsigned long testWifiStart = 0;
const char* version = "2.0.0a";
char selectedProgram[20];
String weatherApiKey = "";
String weatherLocation = "";
String weatherDesc = "";
String weatherIcon = "";
float weatherTemp = 0.0;
float weatherRainChance = 0.0;
int weatherId = 0;
bool useOWA = false;
int rainChanceCutoff = 75;
unsigned long lastWeatherCheck = -900000;
const char * menu[3][5] = {
  {"CANCEL PROGRAMME","START PROGRAMME","REBOOT UNIT","SW VERSION","EXIT MENU"} 
};

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
AsyncWebServer server(WEBSERVPORT);
AsyncEventSource events("/events");
SPRINKLERSYSTEM sprinklersystem(PF575I2C,LCDI2C,LCDROWS,LCDCOLS);
void startup();                         // Pre-declaration for simplicity
String sha1(String payloadStr);
TaskHandle_t mainPollhandle     = NULL;
TaskHandle_t everyminutehandle  = NULL;
TaskHandle_t rotaryLoophandle   = NULL;

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[getWeather]---Fetch weather from OpenWeatherMap-------------]
//////////////////////////////////////////////////////////////////////////
void getWeather() {
  if (weatherApiKey.length() == 0 || weatherLocation.length() == 0) return;
  if ((millis() - lastWeatherCheck) < 900000) return; // Check every 15 mins
  if (WiFi.status() != WL_CONNECTED) return;
  HTTPClient http;
  // Use Forecast API, cnt=8 limits response to next 24 hours (3h * 8)
  String url = "http://api.openweathermap.org/data/2.5/forecast?q=" + weatherLocation + "&appid=" + weatherApiKey + "&units=metric&cnt=8";
  int httpCode = 0;
  int retries = 0;
  while (retries < 3) {
    http.begin(url);
    httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) break;
    http.end();
    retries++;
    delay(1000);
  }
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096); // Increase buffer for forecast data
    deserializeJson(doc, payload);
    if(doc.containsKey("list")){
      // Get current-ish conditions from the first forecast block
      weatherDesc = doc["list"][0]["weather"][0]["description"].as<String>();
      weatherIcon = doc["list"][0]["weather"][0]["icon"].as<String>();
      weatherTemp = doc["list"][0]["main"]["temp"];
      weatherId = doc["list"][0]["weather"][0]["id"];    
      // Calculate max rain chance (pop) in the next 24h
      float maxPop = 0.0;
      for (int i = 0; i < 8; i++) {
        float pop = doc["list"][i]["pop"]; // pop is 0.0 to 1.0
        if (pop > maxPop) maxPop = pop;
      }
      weatherRainChance = maxPop * 100.0;
      lastWeatherCheck = millis();
    }
  }
  http.end();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[logger]-------Push log entry to web browser as event--------]
//////////////////////////////////////////////////////////////////////////
void logger(String message) {
  char now[120];
  strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S - ", &timeinfo);
  String str(now);
  String full_message = str + message;
  events.send(full_message.c_str(),"message",millis());

  // Recovery: If log.txt is missing but log.tmp exists (failed rotation), restore it.
  if (!SPIFFS.exists("/log.txt") && SPIFFS.exists("/log.tmp")) {
      SPIFFS.rename("/log.tmp", "/log.txt");
  }

  File logFile = SPIFFS.open("/log.txt", "a");
  if (logFile) {
    logFile.println(full_message);
    Serial.printf("Logged to SPIFFS: %s\n", full_message.c_str());
    size_t fSize = logFile.size();
    logFile.close();
    // Trim the log file if it gets too big (keep last 10KB if > 20KB)
    if (fSize > 20480) { 
      File readFile = SPIFFS.open("/log.txt", "r");
      if (readFile) {
        File tempFile = SPIFFS.open("/log.tmp", "w");
        if (tempFile) {
          readFile.seek(fSize - 10240);
          while(readFile.available() && readFile.read() != '\n'); // Discard partial line
          while(readFile.available()) tempFile.write(readFile.read());
          tempFile.close();
          readFile.close();
          SPIFFS.remove("/log.txt");
          SPIFFS.rename("/log.tmp", "/log.txt");
        } 
        else { readFile.close(); }
      }
    }
  }
 }  
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[getContentType]----file type handler------------------------]
//////////////////////////////////////////////////////////////////////////
String getContentType(String filename) {
  if (filename.endsWith(F(".htm")))       return F("text/html");
  else if (filename.endsWith(F(".html"))) return F("text/html");
  else if (filename.endsWith(F(".css")))  return F("text/css");
  else if (filename.endsWith(F(".js")))   return F("application/javascript");
  else if (filename.endsWith(F(".json"))) return F("application/json");
  else if (filename.endsWith(F(".png")))  return F("image/png");
  else if (filename.endsWith(F(".gif")))  return F("image/gif");
  else if (filename.endsWith(F(".jpg")))  return F("image/jpeg");
  else if (filename.endsWith(F(".jpeg"))) return F("image/jpeg");
  return F("text/plain");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[cookieParser]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void cookieParser(char (& Array)[4][70], const char * input){
  int currentCookie = 0, count = 0;
  int len = strlen(input);
  for(int i=0; i< len; i++){
    if (input[i] == ';') {
      Array[currentCookie][count] = '\0';
      count = 0;
      currentCookie++;
      if (currentCookie >= 4) break;
      if (i + 1 < len && input[i+1] == ' ') i++; // Skip optional space
      continue;
    }
    if (count < 69) Array[currentCookie][count++] = input[i];
  }
  Array[currentCookie][count] = '\0';
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[getCookieUser]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void getCookieUser(char * output, const char * input){ 
  char fString[4][70] = {0};   
  cookieParser(fString,input);
  char *cookieuser[10] = {0};
  for (auto val : fString) {
    if (strncmp("USER=",val,5) == 0){ 
      char *ptr = NULL;
      byte index = 0; 
      ptr = strtok(val, "=");
      while(ptr != NULL && index < 10){
        cookieuser[index] = ptr;
        index++;
        ptr = strtok(NULL, "=");  // delimiters
      }
    }  
  }
  if (cookieuser[1]) strcpy(output,cookieuser[1]);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [readConfigFile] - [Returns key pair values from cfg files-]
//////////////////////////////////////////////////////////////////////////
void readConfigFile(char* value, const char* filename, const char* parameter){
  File file = SPIFFS.open(filename,"r");
  if (file){
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    if (!error && doc.containsKey(parameter)) {
      const char * source = doc[parameter];  
      if (source) {
        size_t len = strlen(source);
        if (len > 49) len = 49;
        memcpy(value, source, len);
        value[len] = '\0';
      }
    }
    file.close();
  }  
}
// ----------------------------------------------------------------------] 

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [writeConfigFile] - [Adds or updates key pair values in cfg files-]
//////////////////////////////////////////////////////////////////////////
void writeConfigFile(const char* key, const char* filename, const char* value){ 
  DynamicJsonDocument doc(8192);
  File infile = SPIFFS.open(filename, "r");
  if (infile) {
    deserializeJson(doc, infile);
    infile.close();
  }
  // set or replace the key with the provided value
  doc[key] = value;
  File outfile = SPIFFS.open(filename, "w");
  if (outfile) {
    serializeJson(doc, outfile);
    outfile.close();
  }
}
// ----------------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [loadConfig] - [Returns the current version from the Object------]
//////////////////////////////////////////////////////////////////////////
void loadConfig(){  
  sprinklersystem.lcdClearRow(3);
  sprinklersystem.lcdClearRow(1);
  sprinklersystem.lcdClearRow(2);
  sprinklersystem.lcdPrint("clr",0,1,"LOADCFG:");
  File file = SPIFFS.open("/system.cnf","r");
  sprinklersystem.lcdPrint("clr",0,2,"/system.cnf"); 
  DynamicJsonDocument doc(8192);
  deserializeJson(doc, file);  
  file.close();  
  int saveinterval = 0;  
  const char* mtrio = doc["mtrio"];
  const char* meas = doc["meas"];
  const char* deb = doc["mtrdeb"];
  const char* inc = doc["mtrinc"];
  sprinklersystem.addMeter(atoi(mtrio),1,meas[0],atoi(deb),0,atof(inc),saveinterval,true);
  const char* valopio = doc["valopio"];
  const char* valclio = doc["valclio"];
  const char* valrlyio = doc["valrlyio"];
  const char* valrainio = doc["rainsens"];
  sprinklersystem.addValve(atoi(valrlyio),atoi(valopio),atoi(valclio),1);
  if (strcmp(valrainio,"")!=0){   
     sprinklersystem.addRainSensor(atoi(valrainio));
  }
  if (doc.containsKey("firebase_database_url")) {
    const char* fb_db_url = doc["firebase_database_url"];
    const char* fb_secret = doc["firebase_database_secret"];
    sprinklersystem.addFirebase(fb_db_url,fb_secret);
  }
  File zfile = SPIFFS.open("/zones.cnf", "r");
  if (zfile) {
    DynamicJsonDocument zdoc(8192);
    deserializeJson(zdoc, zfile);
    zfile.close();
    JsonArray arr = zdoc["zones"].as<JsonArray>();
    for (JsonVariant value : arr) {
      JsonArray thiszone = value.as<JsonArray>();
      sprinklersystem.addZone(atoi(thiszone[0]), strdup(thiszone[1]));
      sprinklersystem.setDescription(thiszone[1], strdup(thiszone[2]));
    }
  }
  if (doc.containsKey("program")){
    const char* program = doc["program"];
    sprinklersystem.setProgram(atoi(program));
  }
  if (doc.containsKey("rainsensstatus")){
    const char* rainsensstatus = doc["rainsensstatus"]; 
    sprinklersystem.setRainsensorStatus(atoi(rainsensstatus));
  }
  else {      
    if (strcmp(valrainio,"")!=0){    } 
  }
  if (doc.containsKey("weather_api_key")) weatherApiKey = doc["weather_api_key"].as<String>();
  if (doc.containsKey("weather_location")) weatherLocation = doc["weather_location"].as<String>();
  if (doc.containsKey("use_owa")) useOWA = doc["use_owa"];
  if (doc.containsKey("rain_chance_cutoff")) rainChanceCutoff = doc["rain_chance_cutoff"];
  const char* tzData = doc["timez"];
  setenv("TZ", tzData, 1 );
  tzset();
  sprinklersystem.loadMeter(); 
}
// ----------------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[isAut]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool isAuth(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    String cookie = request->header("Cookie");
    char user[20] = {0};
    getCookieUser(user,request->header("Cookie").c_str()); 
    if (strlen(user) == 0) return false;
    char filepwd[50] = {0};
    readConfigFile(filepwd,"/accounts.cnf",user);
    String convFilePwd = filepwd;
    String cookieUser = user;
    String token = sha1(String(cookieUser) + ":" + String(convFilePwd));
    if (cookie.indexOf("SESSIONID=" + token) != -1) {
      return true;
    }
  }
  return false;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[startZone]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool startZone(const char * zone) {
  const char * currentZone = sprinklersystem.isEnabled();
  if (strcmp(zone, "-1") == 0) {
    // Do not start a zone if the name is invalid
    return false;
  }
  if (zoneChangeFlag){
    zoneChangeFlag = false;
  }
  else if (strcmp(currentZone,zone)==0){
    return 0;   
  }
  String message = "Starting zone -> ";
  message.concat(zone);
  logger(message);
  unsigned long tst;
  time_t now;
  getLocalTime(&timeinfo);
  tst = time(&now);
  sprinklersystem.runZone(zone, tst);
  lastTimepoll = -25000;
  events.send("1","stats",millis());
  return 0;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleFileRead]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool handleFileRead(AsyncWebServerRequest *request, String path) {
  if (!isAuth(request)) {
    if (path.endsWith("/") || path.endsWith(".html") || path.endsWith(".htm")) {
      path = "/login.html";
    }
  } 
  else {
    if (path.endsWith("/")) path += F("index.html"); // If a folder is requested, send the index file
  }
  String contentType = getContentType(path);              // Get the MIME type
  String pathWithGz = path + F(".gz");
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){   // If the file exists, either as a compressed archive, or normal
    // Check for redirects BEFORE creating response or modifying path for GZ
    if(path == "/index.html"){
      if (!SPIFFS.exists("/system.cnf")){  
        request->redirect("/configure.html?newcnf=1");
        return true;
      }
      if (!SPIFFS.exists("/programmes.cnf")){ 
        request->redirect("/programme.html?inc=1");
        return true;
      } 
    }
    bool gzipped = false;
    if(SPIFFS.exists(pathWithGz)) {                       // If there's a compressed version available
      path += F(".gz");                                   // Use the compressed version
      gzipped = true;
    }
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);
    if (gzipped){
      response->addHeader("Content-Encoding", "gzip");
    }
    response->addHeader("Cache-Control", "no-cache");  
    request->send(response);
    return true;
  }
  return false;  
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleNotFound]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleNotFound(AsyncWebServerRequest *request) {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += request->url();
  message += "\nMethod: ";
  message += (request->method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += request->args();
  message += "\n";
  for (uint8_t i = 0; i < request->args(); i++) {
    message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
  }
  request->send(404, "text/plain", message);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleLogin]------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleLogin(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    String cookie = request->header("Cookie");
  }
  String user = request->arg("username");
  if (user.length()<1){
    AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
    response->addHeader("Location", "/login.html?msg=e1");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
  else {
    char filepwd[50] = {0};
    readConfigFile(filepwd,"/accounts.cnf",user.c_str());
    String convFilePwd = filepwd;
    if (convFilePwd.length() > 0 && convFilePwd == request->arg("password")){
      AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
      if (!SPIFFS.exists("/system.cnf")){  
        response->addHeader("Location", "/configure.html?newcnf=1");
      } else if (!SPIFFS.exists("/programmes.cnf")){ 
        response->addHeader("Location", "/programme.html?inc=1");
      } else {
        response->addHeader("Location", "/");
      }
      response->addHeader("Cache-Control", "no-cache");
      String token = sha1(request->arg("username") + ":" + convFilePwd);
      response->addHeader("Set-Cookie", "SESSIONID=" + token + "; Path=/");
      response->addHeader("Set-Cookie", "USER=" + request->arg("username") + "; Path=/", false);
      request->send(response);
      return;
    }
    AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
    response->addHeader("Location", "/login.html?msg=e2");
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleLogout]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleLogout(AsyncWebServerRequest *request) {
  AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
  response->addHeader("Location", "/login.html?msg=User disconnected");
  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("Set-Cookie", "SESSIONID=0; Path=/");
  request->send(response);
  return;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handlePullStats]--------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handlePullStats(AsyncWebServerRequest *request) {
  updatepullstatsflag = request->arg("statsdays").toInt();
  request->send(200, "text/html", "{\"s\":\"1\"}");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCancelrun]--------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCancelrun(AsyncWebServerRequest *request) { 
  setCancelFlag = true;
  request->send(200, "text/html", "{\"e\":\"np\"}");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCfgRoot]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCfgRoot(AsyncWebServerRequest *request) { 
  Serial.println("[CfgRoot] Serving cfgindex.html");
  request->send(SPIFFS, "/cfgindex.html");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCheckStatus]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCheckStatus(AsyncWebServerRequest *request) {
  if (isTestingWifi) {
    if (WiFi.status() == WL_CONNECTED || WiFi.localIP() != IPAddress(0,0,0,0)) {
      if (SPIFFS.exists("/testnetwork.cnf")) {
        SPIFFS.remove("/network.cnf");
        SPIFFS.rename("/testnetwork.cnf", "/network.cnf");
      }
      File resultfile = SPIFFS.open("/testresult.cnf", "w");
      if (resultfile) {
        resultfile.print("{\"CONN\":\"SUCCESS\"}");
        resultfile.close();
      }
      isTestingWifi = false;
      shouldRestart = true;
      restartTime = millis();
      request->send(200, "text/html", "SUCCESS");
    } 
    else if (millis() - testWifiStart > 30000) {
      File resultfile = SPIFFS.open("/testresult.cnf", "w");
      if (resultfile) {
        resultfile.print("{\"CONN\":\"FAIL\"}");
        resultfile.close();
      }
      isTestingWifi = false;
      request->send(200, "text/html", "FAIL");
    } 
    else {
      request->send(200, "text/html", "WAIT");
    }
  } 
  else {
    if (SPIFFS.exists("/testresult.cnf")) {
      char result[50] = {0};
      readConfigFile(result,"/testresult.cnf","CONN");      
      request->send(200, "text/html", result);
    } 
    else {
      request->send(200, "text/html", "IDLE");
    }
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleUpdateConfig]-----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleUpdateConfig(AsyncWebServerRequest *request) {
  const char *filename = "/programmes.cnf";
  if (request->arg("event") == "progchange") { //programme change
    if (!SPIFFS.exists("/programmes.cnf")){  
      logger("Error. Programmes not yet configured.");
      request->send(200, "text/html", "{\"s\":\"1\"}");
      return;
    }  
    const char * val = request->arg("value").c_str();   
    writeConfigFile("program","/system.cnf",val); 
    sprinklersystem.setProgram(atoi(val));
    request->send(200, "text/html", "{\"s\":\"0\"}");
    return;
  }
  if (request->arg("event") == "rainsensechange") { //rainsensestatus change
    const char * val = request->arg("value").c_str();    
    writeConfigFile("rainsensstatus","/system.cnf",val); 
    sprinklersystem.setRainsensorStatus(atoi(val));
    request->send(200, "text/html", "{\"s\":\"0\"}");
    logger("Rain Sensor Status Toggled");
    return;
  }
  if (request->arg("event") == "owachange") { //owa change
    const char * val = request->arg("value").c_str();    
    writeConfigFile("use_owa","/system.cnf",val); 
    useOWA = atoi(val);
    request->send(200, "text/html", "{\"s\":\"0\"}");
    logger("OWA Rain Delay Status Toggled");
    return;
  }
  if(!request->hasArg("testval")){
    request->send(400, "text/plain", "Missing configuration data");
    return;
  }
  {
    Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
    DynamicJsonDocument doc(16384);
    DeserializationError error = deserializeJson(doc, request->arg("testval").c_str());
    if (error) {
      Serial.printf("JSON Error: %s\n", error.c_str());
      request->send(500, "text/plain", "JSON Deserialization Error");
      return;
    }
    if (request->arg("filetype") == "zones") {
      filename = "/zones.cnf";
    }
    if (request->arg("filetype") == "cfg") {
      filename = "/system.cnf";
      const char* password = doc["pass"];
      if (password && strlen(password) > 0){
        DynamicJsonDocument accDoc(512);
        File accFile = SPIFFS.open("/accounts.cnf", "r");
        if (accFile) {
            deserializeJson(accDoc, accFile);
            accFile.close();
        }
        accDoc["admin"] = password;
        File accOut = SPIFFS.open("/accounts.cnf", "w");
        if (accOut) {
            serializeJson(accDoc, accOut);
            accOut.close();
        }
      }
      if(sprinklersystem.getProgram() > 0){
        char thisProg[3];
        itoa(sprinklersystem.getProgram(),thisProg,10);
        doc["program"] = thisProg;
      }
      doc.remove("pass");
    }
    File resultfile = SPIFFS.open(filename,"w");
    if (resultfile) {
      serializeJson(doc, resultfile);
      resultfile.close();
    } 
    else {
      request->send(500, "text/plain", "File Write Error");
      return;
    }
  }

  request->send(200, "text/html", "{\"s\":\"0\"}");
  if(!request->hasArg("norestart")){
    shouldRestart = true;
    restartTime = millis();
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleUpdateStatus]-----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleUpdateStatus(AsyncWebServerRequest *request) {
  StaticJsonDocument<512> doc;
  String output;
  int i = sprinklersystem.timeToProgStart(&timeinfo); 
  doc["valve"] = sprinklersystem.valvePosition();
  doc["rsstate"] = sprinklersystem.readRainSensor();
  doc["rsexist"] = sprinklersystem.getHasRainSensor();
  doc["dbactive"] = sprinklersystem.getDatabaseActive();
  doc["manual"] = sprinklersystem.isInManualProgram();
  doc["w_desc"] = weatherDesc;
  doc["w_temp"] = weatherTemp;
  doc["w_icon"] = weatherIcon;
  doc["use_owa"] = useOWA;
  doc["w_pop"] = weatherRainChance;
  doc["rain_cutoff"] = rainChanceCutoff;
  doc["owa_setup"] = (weatherApiKey.length() > 0 && weatherLocation.length() > 0);
  bool physicalRain = (sprinklersystem.getHasRainSensor() && sprinklersystem.getRainSensor() && sprinklersystem.readRainSensor() == 0);
  bool owaRain = (useOWA && ((weatherId >= 200 && weatherId <= 531) || (weatherRainChance >= rainChanceCutoff)));
  if(sprinklersystem.getProgram()==4){doc["state"] = "0";} //programme off
  else if(!sprinklersystem.isSchedForToday(&timeinfo)){doc["state"] = "1";} //Idle, not today
  else if(sprinklersystem.isCanceled()){doc["state"] = "2";} // Manual cancellation
  else if(i>0){
    doc["state"] = "3";
    doc["info"] = i;
  } //countdown to start
  else if(sprinklersystem.isTodayComplete(&timeinfo)){doc["state"] = "4";} //completed for today 
  //elsif made it this far but rain delay state = 6
  else if ((physicalRain || owaRain) && (!sprinklersystem.isInManualProgram()) ){doc["state"] = "6";}
  else {
    doc["state"] = "5";
    const char * zoneName = sprinklersystem.getSchedZone(i);
    int zr = sprinklersystem.getZoneRemaining();
    doc["zone"] = zoneName;
    doc["info"] = zr;
  }//currently running
  serializeJson(doc, output);
  request->send(200, "application/json", output);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetconf]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetconf(AsyncWebServerRequest *request) {
  char user[20];
  const char *filename = "/programmes.cnf";
  if (request->arg("filetype") == "cfg") {
    filename = "/system.cnf";
  }
  if (request->arg("filetype") == "zones") {
    filename = "/zones.cnf";
  }
  getCookieUser(user,request->header("Cookie").c_str());
    if (strcmp(user,"admin")==0){   
      request->send(SPIFFS, filename);          
  }
  else {
    request->send(200, "text/html", "{\"e\":\"np\"}");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetprogram]-------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetprogram(AsyncWebServerRequest *request) {
  String S = "{\"p\":\""+String(sprinklersystem.getProgram())+"\"}";
  request->send(200, "text/html", S);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetrainSense]-----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetrainSense(AsyncWebServerRequest *request) {
  String S = "{\"p\":\""+String(sprinklersystem.getRainSensor())+"\"}";
  request->send(200, "text/html", S);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleReadMeter]--------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleReadMeter(AsyncWebServerRequest *request) {
  String S = "{\"p\":\""+String(sprinklersystem.readMeter())+"\"}";
  request->send(200, "text/html", S);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetLogs]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetLogs(AsyncWebServerRequest *request){
  if (SPIFFS.exists("/log.txt")) {
    request->send(SPIFFS, "/log.txt", "text/plain");
  } else if (SPIFFS.exists("/log.tmp")) {
    request->send(SPIFFS, "/log.tmp", "text/plain");
  } else {
    request->send(200, "text/plain", "");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetZoneList]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetZoneList(AsyncWebServerRequest *request) {
  char nameList[12][20];
  sprinklersystem.getZoneNames(nameList);
  int lastOption = sprinklersystem.getZoneCount();
  String s = "[";
  for (int i = 0; i < lastOption; ++i) {
    const char * desc = sprinklersystem.getDescription(nameList[i]);
    int port = sprinklersystem.getPort(nameList[i]);
    s=s+"{\"name\":\""+desc+"\",\"port\":\""+port+"\",\"val\":\""+nameList[i]+"\"},";        
  } 
  int lastIndex = s.length() - 1;
  s.remove(lastIndex);
  s+="]";
  request->send(200, "text/html", s);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleSendManual]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleSendManual(AsyncWebServerRequest *request) {
  const char * val = request->arg("zone").c_str();   
  setManualFlag = true;
  strncpy(selectedProgram, val, sizeof(selectedProgram) - 1);
  selectedProgram[sizeof(selectedProgram) - 1] = 0; // Ensure null termination
  request->send(200, "text/html", "{\"status\":\"ok\"}");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleSubmitNewMeter]---------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleSubmitNewMeter(AsyncWebServerRequest *request) {
  const char * val = request->arg("value").c_str();   
  sprinklersystem.setMeter(atof(val)); 
  request->send(200, "text/html", "{\"status\":\"ok\"}");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[sha1]-------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
String sha1(String payloadStr){
  const char *payload = payloadStr.c_str(); 
  int size = 20;
  byte shaResult[size];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA1;
  const size_t payloadLength = strlen(payload);
  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char *) payload, payloadLength);
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);
  String hashStr = "";
  for(uint16_t i = 0; i < size; i++) {
    String hex = String(shaResult[i], HEX);
    if(hex.length() < 2) {
      hex = "0" + hex;
    }
    hashStr += hex;
  }
  return hashStr;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiConnect]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiConnect(AsyncWebServerRequest *request) { 
  File testfile = SPIFFS.open("/testnetwork.cnf","w");
  if (!testfile){
      request->send(500, "text/plain", "FS_ERROR");
      return;  
  }
  if (!request->hasParam("ssidval", true) || !request->hasParam("keyval", true)) {
      request->send(400, "text/plain", "BAD_PARAMS");
      testfile.close();
      return;
  }
  String ssid = request->getParam("ssidval", true)->value();
  String pass = request->getParam("keyval", true)->value();
  testfile.print("{\"SSID\":\""); testfile.print(ssid);
  testfile.print("\",\"PASSWORD\":\""); testfile.print(pass); testfile.print("\"}"); 
  testfile.close();
  if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
  WiFi.begin(ssid.c_str(), pass.c_str());
  isTestingWifi = true;
  testWifiStart = millis();
  request->send(200, "text/html", "RCVD");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleGetHistory]-------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetHistory(AsyncWebServerRequest *request) {
  int days = 30;
  if (request->hasArg("days")) {
    days = request->arg("days").toInt();
  }
  String history = sprinklersystem.getHistory(days);
  request->send(200, "application/json", history);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleDeleteHistory]----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleDeleteHistory(AsyncWebServerRequest *request) {
  sprinklersystem.deleteHistory();
  request->send(200, "text/html", "{\"status\":\"ok\"}");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiList]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiList(AsyncWebServerRequest *request) {
  WiFi.scanNetworks(false,false,true);
  int16_t n = WiFi.scanComplete();
  String s = "[";
  for (int i = 0; i < n; ++i) {
    s=s+"{\"name\":\""+WiFi.SSID(i)+"\",\"val\":\""+WiFi.SSID(i)+"\"},";        
  } 
  int lastIndex = s.length() - 1;
  s.remove(lastIndex);
  s+="]";
  request->send(200, "text/html", s);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[serverRouting]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void serverRouting() {
  // Specific API and page handlers
  server.on("/cancelRun",     HTTP_POST, handleCancelrun);
  server.on("/login",         HTTP_POST, handleLogin);
  server.on("/logout",        HTTP_GET,  handleLogout);
  server.on("/updateConfig",  HTTP_POST, handleUpdateConfig);
  server.on("/getConf",       HTTP_POST, handleGetconf);
  server.on("/getProg",       HTTP_POST, handleGetprogram);
  server.on("/getRainsense",  HTTP_POST, handleGetrainSense);
  server.on("/getZoneList",   HTTP_GET,  handleGetZoneList);
  server.on("/readMeter",     HTTP_POST, handleReadMeter);
  server.on("/sendManual",    HTTP_POST, handleSendManual);
  server.on("/sendNewMeter",  HTTP_POST, handleSubmitNewMeter);
  server.on("/updateStatus",  HTTP_POST, handleUpdateStatus);
  server.on("/metricRequest", HTTP_POST, handlePullStats);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect);
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus);
  server.on("/getHistory",    HTTP_GET, handleGetHistory);
  server.on("/deleteHistory", HTTP_POST, handleDeleteHistory);
  server.on("/getLogs",       HTTP_GET, handleGetLogs);
  // Static file handlers (catch-alls) should be last
  server.serveStatic("/configuration.json", SPIFFS, "/configuration.json", "no-cache, no-store, must-revalidate");
  server.onNotFound([](AsyncWebServerRequest *request) {
    if (!handleFileRead(request, request->url())) handleNotFound(request);
  });
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[configNetwork]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void configNetwork() { 
  sprinklersystem.lcdPrint("cls",0,0,"RubinTech");
  sprinklersystem.lcdPrint("clr",0,1,"WIFI: sprinklernet");
  sprinklersystem.lcdPrint("clr",0,2,"Configure at");
  sprinklersystem.lcdClearRow(3);
  sprinklersystem.lcdPrint("clr",0,3,deviceName);
  sprinklersystem.lcdPrintConcat(".local");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  if (!MDNS.begin(deviceName)){
  }
  WiFi.scanNetworks();
  if (SPIFFS.exists("/testnetwork.cnf")){  
    char ssid[50];  
    readConfigFile(ssid,"/testnetwork.cnf","SSID");       
    char key[50];
    readConfigFile(key,"/testnetwork.cnf","PASSWORD");
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    File resultfile = SPIFFS.open("/testresult.cnf","w");
    if (!resultfile){
      return;  
    }    
    WiFi.begin(ssid,key);
    int i =0; 
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      i++;
      if (i==15){
        delay(100);
        SPIFFS.remove("/testnetwork.cnf");
        resultfile.println("{\"CONN\":\"FAIL\"}");
        resultfile.close();            
        startup();
        return;
      }
    }
    resultfile.println("{\"CONN\":\"SUCCESS\"}");
    resultfile.close();
    SPIFFS.rename("/testnetwork.cnf", "/network.cnf");
  }
  Serial.print("hcfgoot");
  server.on("/", HTTP_GET , handleCfgRoot);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[loadNetwork]------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loadNetwork() {
  char ssid[50] = {0}; 
  readConfigFile(ssid,"/network.cnf","SSID");  
  sprinklersystem.lcdPrint("clr",0,2,"Connecting");
  sprinklersystem.lcdClearRow(3);
  char key[50] = {0};
  readConfigFile(key,"/network.cnf","PASSWORD");  
  if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
   WiFi.begin(ssid,key);
  int i =0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    sprinklersystem.lcdPrint("none",i,3,".");
    i++;
    if (i==15){
      sprinklersystem.lcdPrint("clr",0,2,"Connect Fail");
      sprinklersystem.lcdPrint("clr",0,3,"Restart....");
      delay(100);    
      startup();        
    }
  }
  if (!MDNS.begin(deviceName)){
    sprinklersystem.lcdPrint("clr",0,3,"mDNS: Error Starting.");
  }
  else {
    sprinklersystem.lcdPrint("clr",0,3,deviceName);
    sprinklersystem.lcdPrintConcat(".local");
  }
  IPAddress ip = WiFi.localIP();
  char * ourIP = new char[20]();
  sprintf(ourIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  sprinklersystem.lcdPrint("clr",0,2,ourIP);
  delay(1000);  
  sprinklersystem.lcdPrint("clr",0,1,"NTP: ");
  configTime(0, 0, ntpServer);
  if (getLocalTime(&timeinfo)){
    sprinklersystem.lcdPrint("none",5,1,"OK");
  }
  else {
    sprinklersystem.lcdPrint("none",5,1,"FAILED");
    delay(1000);
    ESP.restart();
  }
}
//-----------------------------------------------------------------------]

/////////////////////////////////////////////////////////////////////////
// FUNCTION - [acctMgr] - [Initializes and handles user accounts--------] lib
/////////////////////////////////////////////////////////////////////////
void acctMgr(const char* action,const char* account,const char* val){
 bool shouldCreate = !SPIFFS.exists("/accounts.cnf");
 if (!shouldCreate) {
    File f = SPIFFS.open("/accounts.cnf", "r");
    if (f) {
       if (f.size() == 0) {
         shouldCreate = true;
       } else {
         DynamicJsonDocument doc(256);
         DeserializationError error = deserializeJson(doc, f);
         if (error) shouldCreate = true;
       }
       f.close();
    } else {
      shouldCreate = true;
    }
 }
 if(shouldCreate){ 
    File accountfile = SPIFFS.open("/accounts.cnf","w");
    if (!accountfile){
      return;  
    }
    if ((strcmp(action,"inspect")==0) && (strcmp(account,"admin")==0) && (strcmp(val,"0")==0)){
      accountfile.print("{\"admin\":\"password\"}"); 
      accountfile.close();
    } 
    return;
  }
}
// ---------------------------------------------------------------------]

/////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayheader]---------------------------------------------] lib?
/////////////////////////////////////////////////////////////////////////
void displayHeader() {
  lastTimepoll = millis();
  getLocalTime(&timeinfo);
  char ourtime[20];
  strftime(ourtime, sizeof ourtime, "%A  %H:%M", &timeinfo); 
  if (!displayLock){
    sleep(1);
    sprinklersystem.lcdPrint("init",0,0,ourtime);
    sprinklersystem.lcdPrint("clr",0,1,"Program");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayMeter]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayMeter() {
  if(sprinklersystem.isActive()){
    char mtype = sprinklersystem.getMeasureType();
    double val = sprinklersystem.readConsumption(); 
    char message[30];
    sprintf(message, "%g%c", val, mtype);
    sprinklersystem.lcdPrint("clr",0,3,message);
    events.send(message,"meter",millis());
    delay(350);
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[closeZones]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void closeZones(const char* annotation = NULL) {
  String message = "Closure of zones and valve requested";
  if (annotation) {
    message += " [";
    message += annotation;
    message += "]";
  }
  logger(message);
  sprinklersystem.setValve("CLOSED");
  unsigned long tst;
  time_t now;
  getLocalTime(&timeinfo);
  tst = time(&now);
  sprinklersystem.closeZone(sprinklersystem.isEnabled(),tst, annotation);
  sprinklersystem.clearEnabled();
  delay(10000);
  events.send("1","stats",millis());
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[checkMenu]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool checkMenu(){
  String message;
  if ((programMenu >=0) ){
    int lastOption = sprinklersystem.getZoneCount();
    if(programMenu == lastOption){
      programMenu = -1;
      menulevel =-1;
      selectlevel=-1;
    }
  if (strcmp(selectedProgram,"BACK")==0){
    selectlevel=1;
    menulevel =0;
    programMenu = -1;
    return 0;
  }
    displayLock = false;
    menulevel =0;
    programMenu =-1;
    selectlevel =0;  
    setManualFlag = true; // Will be grabbed by the mainPoll task
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"BACK")==0){
    selectlevel--;
    return 0;
  }
  if (strcmp(menu[menulevel][selectlevel],"REBOOT UNIT")==0){
    logger("Reboot Requested...");
    ESP.restart();
  }
  if (strcmp(menu[menulevel][selectlevel],"SW VERSION")==0){
    sprinklersystem.lcdDisplaySwVersion(version);
    displayLock = false;
    menulevel =0;
    selectlevel =0;
    lastTimepoll = -100000;
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"EXIT MENU")==0){
    displayLock = false;
    menulevel =0;
    selectlevel =0;
    lastTimepoll = -100000;
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"CANCEL PROGRAMME")==0){
    message = "Cancelling manual mode, resuming automatic schedule.";
    logger(message);
    sprinklersystem.setCanceled(true);
    displayLock = false;
    sprinklersystem.cancelManual();
    lastTimepoll = -100000;
    menulevel = 0;
    selectlevel = 0;
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"START PROGRAMME")==0){
    programMenu = 0;
    return 0;
  }
  return 0;
} 
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayEachProgram]-----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayEachProgram(){
  char nameList[12][20];
  sprinklersystem.getZoneNames(nameList);
  int lastOption = sprinklersystem.getZoneCount();
  strncpy(nameList[lastOption],"BACK",20);
  if (programMenu >= lastOption) {programMenu = lastOption;}
  strncpy(selectedProgram,nameList[programMenu],20);
  sprinklersystem.lcdPrint("clr",0,2,nameList[programMenu]);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[loadMenu]---------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loadMenu(int level){
  if (level < 0){level=0;}
  if ((programMenu >=0) ){      
    displayEachProgram();
    return;
  }
  if (selectlevel>4){selectlevel=4;}
  if (selectlevel<0){selectlevel=0;}   //!!!!! change this to appropriate max value
  sprinklersystem.lcdPrint("clr",0,2,menu[level][selectlevel]);
} 
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayCountdown]-------------------------------------------] lib?
//////////////////////////////////////////////////////////////////////////
void displayCountdown(int i) {
  if (!displayLock){  
    sprinklersystem.lcdPrint("clr",0,2,"Begins in ");
    if (i > 60){
      int hours = i/60;
      int minutes = i%60;
      sprinklersystem.lcdPrint("clr",0,3,hours);
      sprinklersystem.lcdPrintConcat("HR ");
      sprinklersystem.lcdPrintConcat(minutes);
    }
    else{
      sprinklersystem.lcdPrint("clr",0,3,i);
    }
    sprinklersystem.lcdPrintConcat(" minutes");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayProgram]---------------------------------------------] displaylock in lib
//////////////////////////////////////////////////////////////////////////
void displayProgram() {
  int testprog = sprinklersystem.getProgram();
  if (!displayLock){
    if(testprog==1){sprinklersystem.lcdPrint("none",8,1,"[A] ");}
    if(testprog==2){sprinklersystem.lcdPrint("none",8,1,"[B] ");}
    if(testprog==3){sprinklersystem.lcdPrint("none",8,1,"[C] ");}
    if(testprog==4){sprinklersystem.lcdPrint("none",8,1,"[OFF]");}
  }
  displayprogram = testprog;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleManualsched]------------------------------------------] lib?
//////////////////////////////////////////////////////////////////////////
void handleManualsched(){
  getLocalTime(&timeinfo);
  //Set the Start time to now or the offset based on the zone start
  sprinklersystem.offsetManual(selectedProgram, &timeinfo);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[mainPoll]-------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void mainPoll(void * parameter){
  for (;;){
    if (setManualFlag){  //---HANDLES THE MANUAL PROGRAMME TURN UP PROCESS
      sprinklersystem.isInManualProgram(true);
      sprinklersystem.setManualZoneChange(true);
      zoneChangeFlag = true;
      setManualFlag = false;
      String message = "Starting manual mode execution beginning from zone -> ";
      message.concat(selectedProgram);
      logger(message);
      lastTimepoll = -100000;
    }//----------------------------------------
    if (setCancelFlag){  //---HANDLES THE CANCELLATION PROCESS
      setCancelFlag = false;
      String message = "Cancellation requested....";
      logger(message);
      sprinklersystem.setCanceled(true);
      displayLock = false;
      sprinklersystem.cancelManual();
      menulevel = 0;
      selectlevel = 0;
      lastTimepoll = -100000;
    }//----------------------------------------
  delay(1000);
  }
}  

//////////////////////////////////////////////////////////////////////////
//-TASK-[everyMinute]----------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void everyMinute(void * parameter){
  static bool lastRainDelayState = false;
  for (;;){
    delay(700); //LCD NEEDED
    if ((millis()-lastTimepoll) >= 30000){ //60000 NORM.
      displayHeader();
      displayProgram();
      getWeather();
      if (sprinklersystem.isInManualProgram()){ //HANDLE MANUAL OPS
        handleManualsched();               
      }
      if(sprinklersystem.isCanceled()){ //HANDLE CANCELED        
        if (!displayLock){
          sprinklersystem.lcdPrint("clr",7,3,"CANCELED");
        }
        if(sprinklersystem.isActive()){          
          closeZones();
        }
        sprinklersystem.setValve("CLOSED");       
        if(sprinklersystem.isTodayComplete(&timeinfo) || !sprinklersystem.isSchedForToday(&timeinfo) || sprinklersystem.timeToProgStart(&timeinfo) >0){          
          sprinklersystem.setCanceled(false);
        }
      }
      else if(!sprinklersystem.isInManualProgram() && ((!sprinklersystem.isSchedForToday(&timeinfo)) || //HANDLE IDLE
        sprinklersystem.getProgram()==4)){
        if(sprinklersystem.isActive()){        
          closeZones();
        }
        sprinklersystem.setValve("CLOSED"); 
        if (!displayLock){
          sprinklersystem.lcdPrint("clr",8,3,"IDLE");
        }
      }
      else {
        int i = sprinklersystem.timeToProgStart(&timeinfo);
        const char * zoneName = sprinklersystem.getSchedZone(i);
        if (i > 0) {
          displayCountdown(i);
        } else if (strcmp(zoneName, "-1") == 0) { // isTodayComplete
            if (sprinklersystem.isInManualProgram()){  
              sprinklersystem.cancelManual();
            }           
            if (!displayLock){
              sprinklersystem.lcdPrint("clr",0,2,"Today's Schedule");
              sprinklersystem.lcdPrint("clr",0,3,"has completed..");
            }
            if(sprinklersystem.isActive()){
              closeZones();
            }
        } else { // A zone is scheduled to run
          if(!displayLock){
            //check rain sensor
            bool physicalRain = (sprinklersystem.getHasRainSensor() && sprinklersystem.getRainSensor() && sprinklersystem.readRainSensor() == 0);
            bool owaRain = (useOWA && ((weatherId >= 200 && weatherId <= 531) || (weatherRainChance >= rainChanceCutoff)));
            bool currentRainDelay = (physicalRain || owaRain);
            if (currentRainDelay && !lastRainDelayState) {
                logger("Rain Delay condition detected. Pausing/Skipping schedule.");
            } else if (!currentRainDelay && lastRainDelayState) {
                logger("Rain Delay condition cleared. Resuming schedule.");
            }
            lastRainDelayState = currentRainDelay;
            if (currentRainDelay && (!sprinklersystem.isInManualProgram())) {
              //engaged AND NOT MANUAL MODE
              sprinklersystem.lcdPrint("clr", 3, 2, "--Rain Delay--");
              if (sprinklersystem.isActive()) {
                closeZones("Rain Delay");
              }
            } else {
              int zr = sprinklersystem.getZoneRemaining();
              char thiszr[4];
              itoa(zr, thiszr, 10);
              sprinklersystem.lcdPrint("clr", 0, 2, zoneName);
              sprinklersystem.lcdPrint("na", 12, 2, thiszr);
              sprinklersystem.lcdPrintConcat("min");
              startZone(zoneName); // All the functions for zone open
              displayMeter();
            }
          } 
        }
      } 
    }
    if(sprinklersystem.meterMoved()){
      displayMeter();
    }  
    int testprog = sprinklersystem.getProgram();
    if (displayprogram != testprog){
      lastTimepoll = -60000;
    }
  }
}
//-----------------------------------------------------------------------]  

//////////////////////////////////////////////////////////////////////////
//-TASK-[rotaryLoop]-----------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void rotaryLoop(void * parameter){
  for(;;){
	  if (rotaryEncoder.encoderChanged()){
      if (displayLock){
        int encval = rotaryEncoder.readEncoder();
        bool up = (encval > lastEncval);
        lastEncval = encval;    

        if (programMenu >= 0) {
          int maxZones = sprinklersystem.getZoneCount();
          if (up) {
            if (programMenu < maxZones) programMenu++;
          } else {
            if (programMenu > 0) programMenu--;
          }
        } else {
          if (up) {
            if (selectlevel < 4) selectlevel++;
          } else {
            if (selectlevel > 0) selectlevel--;
          }
        }
        loadMenu(menulevel);       
      }  
	  }
	  if (rotaryEncoder.isEncoderButtonClicked()){      
      if(displayLock){
        bool stophere = checkMenu();
        if (stophere){continue;}
        }      
      if (selectlevel < 0){selectlevel =0;}
      if (!displayLock){
        displayLock=true;
        lastEncval = rotaryEncoder.readEncoder();
        sprinklersystem.lcdPrint("init",7,0,"*MENU*");
      } 
      loadMenu(menulevel);        
	  }
    //uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    //Serial.print("ROTARY-HWM: ");Serial.println(uxHighWaterMark);
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[startup]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void startup(){
  Serial.print("\n\nESP32 Irigation System version ");
  Serial.println(version);
  Serial.print("2023-2026 Jordan Rubin\ndelorean1@gmail.com\n");

  if (SPIFFS.exists("/network.cnf")){  
     sprinklersystem.lcdPrint("clr",0,3,"NETWK: Loading");
     loadNetwork();
  }
  else {
    sprinklersystem.lcdPrint("clr",0,3,"NETWK: No Config");   
     configNetwork();    
  }

  server.on("/getWifiList",  HTTP_GET, handleWifiList);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect); 
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus);

  serverRouting();
  server.addHandler(&events);
  server.begin();
  delay(1000);

  // Initialize OTA
  ArduinoOTA.setHostname(deviceName);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) type = "sketch";
    else type = "filesystem";
    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    logger("OTA Update Start: " + type);
  });
  ArduinoOTA.onEnd([]() { logger("\nOTA Update End"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    // Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) { logger("OTA Error: " + String(error)); });
  ArduinoOTA.begin();


  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
    client->send("INIT", NULL, millis(), 10000);
  });
 
  if (SPIFFS.exists("/system.cnf")){  
    loadConfig();
    delay(500);
    if (!SPIFFS.exists("/programmes.cnf")){  
      sprinklersystem.lcdPrint("clearrow",0,3,"CONFIG INCOMPLETE");
    } 
    else {
      sprinklersystem.lcdPrint("clr",0,3,"PROCESS COMPLETE");   
      delay(1000);
      xTaskCreatePinnedToCore(mainPoll,"mainPolltask",4096,NULL,1,&mainPollhandle,1);
      xTaskCreatePinnedToCore(everyMinute,"everyminutetask",12288,NULL,1,&everyminutehandle,1);
      xTaskCreatePinnedToCore(rotaryLoop,"rotaryLooptask",4096,NULL,1,&rotaryLoophandle,1);
    }   
  }  
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[facdefMonitor]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void facdefMonitor(void * parameter){
  for(;;){
    double now = millis();
    while (digitalRead(FACDEFPIN) == LOW){
      if ((millis()-now) >= FACDEFDELAY){ 
        sprinklersystem.lcdClearRow(1);
        sprinklersystem.lcdClearRow(3);
        sprinklersystem.lcdClearRow(2); 
        sprinklersystem.lcdPrint("clr",0,2,"Defaulting unit!");
        SPIFFS.remove("/network.cnf");
        SPIFFS.remove("/testresult.cnf");
        SPIFFS.remove("/testnetwork.cnf");
        SPIFFS.remove("/configuration.json");
        SPIFFS.remove("/accounts.cnf");
        SPIFFS.remove("/system.cnf");
        SPIFFS.remove("/programmes.cnf");
        SPIFFS.remove("/log.txt");
        sprinklersystem.lcdClearRow(1);
        sprinklersystem.lcdClearRow(2);
        sprinklersystem.lcdClearRow(3);     
        sprinklersystem.lcdPrint("clr",0,3,"DEFAULTED!!!!");
        delay(2000);
        ESP.restart();
      }
   }  
  delay (2000);
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[readEncoderISR]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void IRAM_ATTR readEncoderISR(){
	rotaryEncoder.readEncoder_ISR();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[setup]--------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  TaskHandle_t facdefhandle = NULL;
  pinMode(FACDEFPIN, INPUT_PULLUP);
  sprinklersystem.begin();
  rotaryEncoder.setBoundaries(0,1000,false);
  rotaryEncoder.begin();
  rotaryEncoder.disableAcceleration();
	rotaryEncoder.setup(readEncoderISR);
  xTaskCreatePinnedToCore(facdefMonitor,"facdeftask",2300,NULL,1,&facdefhandle,1);
  startup();
}

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[loop]--------------------STAYS EMPTY--------------------------]
//////////////////////////////////////////////////////////////////////////
void loop() {
  ArduinoOTA.handle();
  if (shouldRestart && (millis() - restartTime > 3000)){
    ESP.restart();
  }
}
//-----------------------------------------------------------------------]