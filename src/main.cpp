/*
  Rubin Projects Boot Framework and ASYNC web Based Wifi configuration Framework 
  Standardized basic framework for my projects that require no special code nor
  any extra libraries beyond what I regularly use in my projects to put an ESP32
  on the network with an interactive Web based GUI. 
  https://www.youtube.com/c/jordanrubin6502
  2022 Jordan Rubin.
*/
#define POWER_MONITOR_PIN 27
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
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "mbedtls/md.h"
#include <HTTPClient.h>
#include <Sprinkler.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <WiFiAP.h>

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
unsigned long powerDebounceTime = 0;
int programMenu = -1;
bool setManualFlag = false;
bool setCancelFlag = false;
bool zoneChangeFlag = false;
bool powerFlag = false;
const char* version = "1.0.0b";
char selectedProgram[20];
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
//-FUNCTION-[logger]-------Push log entry to web browser as event--------]
//////////////////////////////////////////////////////////////////////////
void logger(String message) {
  char now[120];
  strftime(now, sizeof(now), "%Y-%m-%d %H:%M:%S - ", &timeinfo);
  String str(now);
  message = now+message;  
  events.send(message.c_str(),"message",millis());
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
void cookieParser(char (& Array)[4][70],  const char * input){
  int currentCookie = 0, count = 0;
  const char * token = input;
  for(int i=0; i< strlen(token); i++){
    if (((token[i] == ';')&&(token[i+1] == ' ')) || (i==strlen(token))){
      char * returnSlot = Array[currentCookie];
      returnSlot[count] = '\0';
      count = 0;
      currentCookie++;
      i=i+2;   
    }
    char * returnSlot = Array[currentCookie];
    returnSlot[count] = token[i];  
    count++; 
    if (i == (strlen(token)-1)){
      returnSlot[count] = '\0'; 
    }
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[getCookieUser]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void getCookieUser(char * output, const char * input){ 
  char fString[4][70];   
  cookieParser(fString,input);
  char *cookieuser[10];
  for (auto val : fString) {
    if (strncmp("USER=",val,5) == 0){ 
      char *ptr = NULL;
      byte index = 0; 
      ptr = strtok(val, "=");
      while(ptr != NULL){
        cookieuser[index] = ptr;
        index++;
        ptr = strtok(NULL, "=");  // delimiters
      }
    }  
  }
  strcpy(output,cookieuser[1]);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [readConfigFile] - [Returns key pair values from cfg files-]
//////////////////////////////////////////////////////////////////////////
void readConfigFile(char* value, const char* filename, const char* parameter){
  File file = SPIFFS.open(filename,"r");
  if (file){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    if (doc.containsKey(parameter)) {
      const char * source = doc[parameter];  
      strncpy(value, source,strlen(source));
      value[strlen(source)] = '\0';
      file.close();
    }
  }  
}
// ----------------------------------------------------------------------] 

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [writeConfigFile] - [Adds or updates key pair values in cfg files-]
//////////////////////////////////////////////////////////////////////////
void writeConfigFile(const char* value, const char* filename, const char* parameter){ 
  File file = SPIFFS.open(filename,"r");
  if (file){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();
    doc[value] = parameter;
//serializeJson(doc, Serial); //////////////////////////////???????????
    File file = SPIFFS.open(filename,"w");
    serializeJson(doc, file); 
  } 
  file.close();
}
// ----------------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
// FUNCTION - [loadConfig] - [Returns the current version from the Object------]
//////////////////////////////////////////////////////////////////////////
void loadConfig(){  
  sprinklersystem.lcdClearRow(3);
  sprinklersystem.lcdClearRow(1);
  sprinklersystem.lcdClearRow(2);
  sprinklersystem.lcdPrint("clearrow",0,1,"LOADCFG:");
  File file = SPIFFS.open("/system.cnf","r");
  sprinklersystem.lcdPrint("clearrow",0,2,"/system.cnf");
///////////////
//while (file.available()) {
//  Serial.write(file.read());
//}
//file.close();
//file = SPIFFS.open("/system.cnf","r");
/////////////   
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);  
  file.close();  
  int saveinterval = 0;  
  const char* mtrio = doc["mtrio"];
  const char* meas = doc["meas"];
  const char* deb = doc["mtrdeb"];
  const char* inc = doc["mtrinc"];
  sprinklersystem.addMeter(atoi(mtrio),1,meas[0],atoi(deb),1,atof(inc),saveinterval,true);
  const char* valopio = doc["valopio"];
  const char* valclio = doc["valclio"];
  const char* valrlyio = doc["valrlyio"];
  const char* valrainio = doc["rainsens"];
  sprinklersystem.addValve(atoi(valrlyio),atoi(valopio),atoi(valclio),1);
  if (strcmp(valrainio,"")!=0){   
    sprinklersystem.addRainSensor(atoi(valrainio));
  }
  const char* apiKey = doc["detaapikey"];
  const char* detaID = doc["detaid"];
  const char* detaBaseName = doc["detaname"];
  sprinklersystem.addDetabase(detaID,detaBaseName,apiKey);
  JsonArray arr = doc["zones"].as<JsonArray>();
  for (JsonVariant value : arr) {
    JsonArray thiszone = value;
    const char * port = thiszone.getElement(0);
    const char * name = thiszone.getElement(1);
    const char * desc = thiszone.getElement(2);
    char* newname = strdup(name);
    char* newdesc = strdup(desc);  
    sprinklersystem.addZone(atoi(port),newname);
    sprinklersystem.setDescription(name,newdesc);
  }
    if (doc.containsKey("program")){
      const char* program = doc["program"];
      sprinklersystem.setProgram(atoi(program));
    }
    if (doc.containsKey("rainsensstatus")){
      const char* rainsensstatus = doc["rainsensstatus"];
      Serial.print("rainsens key defined "); Serial.println(atoi(rainsensstatus)); 
      sprinklersystem.setRainsensorStatus(atoi(rainsensstatus));
    }
    else {
//Serial.println("rainsens key undefined");       
       if (strcmp(valrainio,"")!=0){  
//Serial.println("rainsens has gpio");       
//           sprinklersystem.setRainsensorStatus(1);
//save value of 1
       } 
    }
   const char* tzData = doc["timez"];
   setenv("TZ", tzData, 1 );
   tzset();
}
// ----------------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[isAut]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool isAuth(AsyncWebServerRequest *request) {
  if (request->hasHeader("Cookie")) {
    String cookie = request->header("Cookie");
    char user[20];
    getCookieUser(user,request->header("Cookie").c_str()); 
    char filepwd[50];
    readConfigFile(filepwd,"/accounts.cnf",user);
    String convFilePwd = filepwd;
    String cookieUser = user;
    String token = sha1(String(cookieUser) + ":" +      
    String(convFilePwd) + ":" + 
    request->client()->remoteIP().toString());
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
  if (zoneChangeFlag){
    zoneChangeFlag = false;
    sprinklersystem.setManualZoneChange(true);
  }
  else if (strcmp(currentZone,zone)==0){
    return 0;   
  }
  String message = "Starting zone -> ";
  message.concat(zone);
  logger(message);
  sprinklersystem.runZone(zone);
  delay(2000);
  lastTimepoll = -25000;
  return 0;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleFileRead]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool handleFileRead(AsyncWebServerRequest *request, String path) {
  if (!isAuth(request)) {
    path = "/login.html";
  } 
  else {
    if (path.endsWith("/")) path += F("index.html"); // If a folder is requested, send the index file
  }
  String contentType = getContentType(path);              // Get the MIME type
  String pathWithGz = path + F(".gz");
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){   // If the file exists, either as a compressed archive, or normal
    bool gzipped = false;
    if(SPIFFS.exists(pathWithGz)) {                       // If there's a compressed version available
      path += F(".gz");                                   // Use the compressed version
      gzipped = true;
    }
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, path, contentType);
    if (gzipped){
      response->addHeader("Content-Encoding", "gzip");
    }
    if(path=="/index.html"){
      if (!SPIFFS.exists("/system.cnf")){  
        request->redirect("/configure.html?newcnf=1");
      }
      else {
        if (!SPIFFS.exists("/programmes.cnf")){ 
          request->redirect("/programme.html?inc=1");
        } 
      }
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
    char filepwd[50];
    readConfigFile(filepwd,"/accounts.cnf",user.c_str());
    String convFilePwd = filepwd;
    if (convFilePwd == request->arg("password")){
      AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
      response->addHeader("Location", "/");
      response->addHeader("Cache-Control", "no-cache");
      String token = sha1(request->arg("username") + ":" + convFilePwd + ":" + request->client()->remoteIP().toString());
      response->addHeader("Set-Cookie", "SESSIONID=" + token);
      response->addHeader("Set-Cookie", "USER=" + request->arg("username"));
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
  response->addHeader("Set-Cookie", "SESSIONID=0");
  request->send(response);
  return;
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
  request->send(SPIFFS, "/cfgindex.html");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleCheckStatus]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleCheckStatus(AsyncWebServerRequest *request) {
  char result[20];
  readConfigFile(result,"/testresult.cnf","CONN");      
  request->send(200, "text/html", result);
  if (strcmp(result,"SUCCESS") == 0){
    ESP.restart();
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
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, request->arg("testval").c_str());
  if (request->arg("filetype") == "cfg") {
    filename = "/system.cnf";
    const char* password = doc["pass"];
    if (strcmp(password,"") != 0){
      writeConfigFile("admin","/accounts.cnf",password);
    }
  if(sprinklersystem.getProgram() > 0){
    char thisProg[3];
    itoa(sprinklersystem.getProgram(),thisProg,10);
    doc["program"] = thisProg;
  }
    doc.remove("pass");
  }
  File resultfile = SPIFFS.open(filename,"w");
  serializeJson(doc, resultfile);
  resultfile.close();
  request->send(200, "text/html", "{\"s\":\"0\"}");
  delay(400);
  ESP.restart();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleUpdateStatus]-----------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleUpdateStatus(AsyncWebServerRequest *request) {
  String state;
  String info;
  String zname;
  String valve;
  String rsState;
  String rsExist;
  int i = sprinklersystem.timeToProgStart(&timeinfo); 
  const char * valvePos = sprinklersystem.valvePosition();
  bool rs = sprinklersystem.readRainSensor();
  bool rsEx = sprinklersystem.getHasRainSensor();
  bool dbActive = sprinklersystem.getDatabaseActive();
  valve = valvePos;
  rsState = rs;
  rsExist = rsEx;
  if(sprinklersystem.getProgram()==4){state="0";} //programme off
  else if(!sprinklersystem.isSchedForToday(&timeinfo)){state ="1";} //Idle, not today
  else if(sprinklersystem.isCanceled()){state ="2";} // Manual cancellation
  else if(i>0){
      state ="3";
      info = i;
  } //countdown to start
  else if(sprinklersystem.isTodayComplete(&timeinfo)){state="4";} //completed for today 
  //elsif made it this far but rain delay state = 6
  else if ((sprinklersystem.readRainSensor()==0) && (!sprinklersystem.isInManualProgram()) ){state="6";}
  else {
      state="5";
      const char * zoneName = sprinklersystem.getSchedZone(i);
      int zr = sprinklersystem.getZoneRemaining();
      zname = zoneName;
      info = zr;
    }//currently running
  String S = "{\"state\":\""+state+"\",\"info\":\""+info+"\",\"rsstate\":\""+rsState+"\",\"rsexist\":\""+rsExist+"\", \"dbactive\":\""+dbActive+"\",\"zone\":\""+zname+"\",\"valve\":\""+valve+"\"}";
  request->send(200, "text/html", S);
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
//-FUNCTION-[handleGetZoneList]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleGetZoneList(AsyncWebServerRequest *request) {
  char nameList[12][20];
  sprinklersystem.getZoneNames(nameList);
  int lastOption = sprinklersystem.getZoneCount();
  String s = "[";
  for (int i = 0; i < lastOption; ++i) {
    const char * desc = sprinklersystem.getDescription(nameList[i]);
    s=s+"{\"name\":\""+desc+"\",\"val\":\""+nameList[i]+"\"},";        
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
  strncpy(selectedProgram,val,20);
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
//-FUNCTION-[serverRouting]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void serverRouting() {
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
  server.onNotFound([](AsyncWebServerRequest *request) {  // If the client requests any URI
    if (!handleFileRead(request, request->url())){        // send it if it exists
      handleNotFound(request); // respond 404 
    }
  });
  server.serveStatic("/configuration.json", SPIFFS, "/configuration.json", "no-cache, no-store, must-revalidate");
  server.serveStatic("/", SPIFFS, "/", "max-age=31536000");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiConnect]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiConnect(AsyncWebServerRequest *request) { 
  File testfile = SPIFFS.open("/testnetwork.cnf","w");
  if (!testfile){
      return;  
  }
  AsyncWebParameter* p = request->getParam(0);
  testfile.print("{\"SSID\":\""); testfile.print(p->value().c_str());
  p = request->getParam(1);
  testfile.print("\",\"PASSWORD\":\""); testfile.print(p->value().c_str()); testfile.print("\"}"); 
  testfile.close();
  request->send(200, "text/html", "RCVD"); 
  startup();
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
//-FUNCTION-[configNetwork]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void configNetwork() { 
  sprinklersystem.lcdPrint("clearscreen",0,0,"RubinTech");
  sprinklersystem.lcdPrint("clearscreen",0,1,"WIFI: sprinklernet");
  sprinklersystem.lcdPrint("clearscreen",0,2,"Configure at");
  sprinklersystem.lcdClearRow(3);
  sprinklersystem.lcdPrint("clearscreen",0,3,deviceName);
  sprinklersystem.lcdPrintConcat(".local");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  if (!MDNS.begin(deviceName)){
//  Serial.print(F("Error starting mDNS"));
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
  server.on("/", HTTP_GET , handleCfgRoot);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[loadNetwork]------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loadNetwork() {
    char ssid[50]; 
    readConfigFile(ssid,"/network.cnf","SSID");  
    sprinklersystem.lcdPrint("clearrow",0,2,"Connecting");
    sprinklersystem.lcdClearRow(3);
    char key[50];
    readConfigFile(key,"/network.cnf","PASSWORD");  
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    WiFi.begin(ssid,key);
    int i =0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      sprinklersystem.lcdPrint("none",i,3,".");
      i++;
      if (i==15){
        sprinklersystem.lcdPrint("clearrow",0,2,"Connect Fail");
        sprinklersystem.lcdPrint("clearrow",0,3,"Restart....");
        delay(100);    
        startup();        
    }
  }
  if (!MDNS.begin(deviceName)){
    sprinklersystem.lcdPrint("clearrow",0,3,"mDNS: Error Starting.");
  }
  else {
    sprinklersystem.lcdPrint("clearrow",0,3,deviceName);
    sprinklersystem.lcdPrintConcat(".local");
  }
  IPAddress ip = WiFi.localIP();
  char * ourIP = new char[20]();
  sprintf(ourIP, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  sprinklersystem.lcdPrint("clearrow",0,2,ourIP);
  delay(1000);  
  sprinklersystem.lcdPrint("clearrow",0,1,"NTP: ");
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
 if(!SPIFFS.exists("/accounts.cnf")){ 
    File accountfile = SPIFFS.open("/accounts.cnf","w");
    if (!accountfile){
      return;  
    }
    if ((strcmp(action,"inspect")==0) && (strcmp(account,"admin")==0) && (strcmp(val,"0")==0)){
      accountfile.println("{\"admin\":\"password\"}"); 
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
//  if (!sprinklersystem.lcdlock){
  lastTimepoll = millis();
  getLocalTime(&timeinfo);
  char ourtime[20];
  strftime(ourtime, sizeof ourtime, "%A  %H:%M", &timeinfo); 
  if (!displayLock){
    sleep(1);
    sprinklersystem.lcdPrint("init",0,0,ourtime);
    sprinklersystem.lcdPrint("clearrow",0,1,"Program");
  }
//  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayMeter]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayMeter() {
//  if (!displayLock){
  if (!sprinklersystem.lcdlock){
    if(sprinklersystem.isActive()){
      char mtype = sprinklersystem.getMeasureType();
      double val = sprinklersystem.readConsumption(); 
      sleep(1);
char * numer = new char[20]();
char *type = new char[2]();
//strncat()
 // sprintf(numer, "0.3f", val);
sprinklersystem.lcdPrint("clearrow",0,3,"numer");

    //  sprinklersystem.lcdPrint("clearrow",0,3,val);
      //lcd.setCursor(0,3);
      //lcd.print(val);
      //lcd.print(mtype);
Serial.println(mtype);
 Serial.println(numer);     
      sprinklersystem.lcdPrintConcat("mtype");
      //lcd.setCursor(0,3);
      char message[30];
      sprintf(message, "%g%c", val, mtype);
      events.send(message,"meter",millis());
      delay(350);
    }
  } 
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[closeZones]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void closeZones() {
  String message = "Closure of zones and valve requested";
  logger(message);
  sprinklersystem.setValve("CLOSED");
  sprinklersystem.closeZone(sprinklersystem.isEnabled());
  sprinklersystem.clearEnabled();
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
  sprinklersystem.lcdPrint("clearrow",0,2,nameList[programMenu]);
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
  sprinklersystem.lcdPrint("clearrow",0,2,menu[level][selectlevel]);
} 
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayCountdown]-------------------------------------------] lib?
//////////////////////////////////////////////////////////////////////////
void displayCountdown(int i) {
  if (!displayLock){  
    sprinklersystem.lcdPrint("clearrow",0,2,"Begins in ");
    if (i > 60){
      int hours = i/60;
      int minutes = i%60;
      sprinklersystem.lcdPrint("clearrow",0,3,hours);
      sprinklersystem.lcdPrintConcat("HR ");
      sprinklersystem.lcdPrintConcat(minutes);
    }
    else{
      sprinklersystem.lcdPrint("clearrow",0,3,i);
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
//-FUNCTION-[checkPower]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void checkPower(){
  Serial.println(digitalRead(POWER_MONITOR_PIN));
  if (digitalRead(POWER_MONITOR_PIN) == LOW){
    Serial.println("Power loss detected....");
    Serial.println("Writing meter to Filesystem.");
    sprinklersystem.updateMeter();
    Serial.println("Disable LCD");
    sprinklersystem.lcdPower(0);
    String message = "Power loss detected, writing to FS and shifting to low power mode.";
    logger(message);
  }
  else{
    Serial.println("Power restored....");
    sprinklersystem.lcdPower(1);
    String message = "Power restored";
    logger(message);
  }
  powerFlag = false;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[mainPoll]-------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void mainPoll(void * parameter){
//UBaseType_t uxHighWaterMark;
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
  //  uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
  //  Serial.print("MAINPT-HWM: ");Serial.println(uxHighWaterMark);
  }
}  
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[everyMinute]----------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void everyMinute(void * parameter){
//UBaseType_t uxHighWaterMark;
  for (;;){
    delay(700); //LCD NEEDED
    if (powerFlag){checkPower();}
    if ((millis()-lastTimepoll) >= 30000){ //60000 NORM.
      displayHeader();
      displayProgram();
      if (sprinklersystem.isInManualProgram()){ //HANDLE MANUAL OPS
        handleManualsched();               
      }
      if(sprinklersystem.isCanceled()){ //HANDLE CANCELED        
        if (!displayLock){
          sprinklersystem.lcdPrint("clearrow",7,3,"CANCELED");
        }
        if(sprinklersystem.isActive()){          
          closeZones();
        }
        sprinklersystem.setValve("CLOSED");       
        if(sprinklersystem.isTodayComplete(&timeinfo) || !sprinklersystem.isSchedForToday(&timeinfo) || sprinklersystem.timeToProgStart(&timeinfo) >0){          
          sprinklersystem.setCanceled(false);
        }
      }
      else if((!sprinklersystem.isSchedForToday(&timeinfo)) || //HANDLE IDLE
        sprinklersystem.getProgram()==4){
        if(sprinklersystem.isActive()){        
          closeZones();
        }
        sprinklersystem.setValve("CLOSED"); 
        if (!displayLock){
          sprinklersystem.lcdPrint("clearrow",8,3,"IDLE");
        }
      }
      else {
        int i = sprinklersystem.timeToProgStart(&timeinfo);
        if (i > 0){
          displayCountdown(i);        
        }
        else { //Todays Activity Arrived or passed
          const char *  zoneName = sprinklersystem.getSchedZone(i); 
          if(sprinklersystem.isTodayComplete(&timeinfo)){
            if (sprinklersystem.isInManualProgram()){  
              sprinklersystem.cancelManual();
            }           
            if (!displayLock){
              sprinklersystem.lcdPrint("clearrow",0,2,"Today's Schedule");
              sprinklersystem.lcdPrint("clearrow",0,3,"has completed..");
            }
            if(sprinklersystem.isActive()){
              closeZones();
            }
          }
          else {
            if(!displayLock){
              //check rain sensor
              if ((sprinklersystem.getHasRainSensor()) && (sprinklersystem.getRainSensor())){     
                if ((sprinklersystem.readRainSensor() ==0) && (!sprinklersystem.isInManualProgram())){  //engaged AND NOT MANUAL MODE
                  sprinklersystem.lcdPrint("clearrow",3,2,"--Rain Delay--");
                  if(sprinklersystem.isActive()){
                    closeZones();
                  }
                  continue;
                }  
              }
              int zr = sprinklersystem.getZoneRemaining();
              //PLACEHOLDER TO KICKOFF THE RUN TASK AND HALT THIS ONE
              char thiszr[4];
              itoa(zr,thiszr,10);
              sprinklersystem.lcdPrint("clearrow",12,2,thiszr);
              sprinklersystem.lcdPrintConcat("min");
              sprinklersystem.lcdPrint("clearrow",0,2,zoneName);
            }
            startZone(zoneName); // All the functions for zone open
            displayMeter();
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
//    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
//    Serial.print("EVERYMIN-HWM: ");Serial.println(uxHighWaterMark);
  }
}
//-----------------------------------------------------------------------]  

//////////////////////////////////////////////////////////////////////////
//-TASK-[rotaryLoop]-----------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void rotaryLoop(void * parameter){
  //UBaseType_t uxHighWaterMark;
  for(;;){
	  if (rotaryEncoder.encoderChanged()){
Serial.print("ENCODER- ");
Serial.print("PM:"); Serial.print(programMenu);
Serial.print(" SL:"); Serial.print(selectlevel);
Serial.print(" ML:"); Serial.println(menulevel);
int txt = rotaryEncoder.readEncoder();
Serial.print(" VAL:"); Serial.println(txt);


      if (displayLock){
        int encval = rotaryEncoder.readEncoder();
        if (encval < lastEncval){
          if (programMenu >0){programMenu--;}
          selectlevel--;
        }
        else {
          if (programMenu >=0){programMenu++;}
          selectlevel++;
        }
        lastEncval = encval;    
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
  acctMgr("inspect","admin","0");
  if (SPIFFS.exists("/network.cnf")){  
     sprinklersystem.lcdPrint("clearrow",0,3,"NETWK: Loading");
     loadNetwork();
  }
  else {
    sprinklersystem.lcdPrint("clearrow",0,3,"NETWK: No Config");   
     configNetwork();    
  }
  server.on("/getWifiList",  HTTP_GET, handleWifiList);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect); 
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus);
  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    // send event with message "hello!", id current millis
    // and set reconnect delay to 1 second
   client->send("INIT", NULL, millis(), 10000);
  });
  serverRouting();
  server.addHandler(&events);
  server.begin();
  delay(2000);
  if (SPIFFS.exists("/system.cnf")){  
    loadConfig();
    delay(500);
    if (!SPIFFS.exists("/programmes.cnf")){  
      sprinklersystem.lcdPrint("clearow",0,3,"CONFIG INCOMPLETE");
    } 
    else {
      sprinklersystem.lcdPrint("clearrow",0,3,"PROCESS COMPLETE");   
      delay(1000);
      xTaskCreatePinnedToCore(mainPoll,"mainPolltask",1600,NULL,1,&mainPollhandle,1);     //132
      xTaskCreatePinnedToCore(everyMinute,"everyminutetask",2600,NULL,1,&everyminutehandle,1); //???
      xTaskCreatePinnedToCore(rotaryLoop,"rotaryLooptask",1800,NULL,1,&rotaryLoophandle,1);
      everyMinute(nullptr);   
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
        sprinklersystem.lcdPrint("clearrow",0,2,"Defaulting unit!");
        SPIFFS.remove("/network.cnf");
        SPIFFS.remove("/testresult.cnf");
        SPIFFS.remove("/testnetwork.cnf");
        SPIFFS.remove("/configuration.json");
        SPIFFS.remove("/accounts.cnf");
        SPIFFS.remove("/system.cnf");
        SPIFFS.remove("/programmes.cnf");
        sprinklersystem.lcdClearRow(1);
        sprinklersystem.lcdClearRow(2);
        sprinklersystem.lcdClearRow(3);     
        sprinklersystem.lcdPrint("clearrow",0,3,"DEFAULTED!!!!");
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
//-TASK-[handlePowerFailureISR]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void IRAM_ATTR handlePowerFailureISR() {
    if ((millis() - powerDebounceTime) > 200) {
      powerFlag = true;
      powerDebounceTime = millis();
    }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[setup]--------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  TaskHandle_t facdefhandle = NULL;
  pinMode(FACDEFPIN, INPUT_PULLUP);
  pinMode(POWER_MONITOR_PIN,INPUT_PULLDOWN);
  sprinklersystem.begin();
  rotaryEncoder.setBoundaries(0,1000,false);
  rotaryEncoder.begin();
  rotaryEncoder.disableAcceleration();
	rotaryEncoder.setup(readEncoderISR);
  xTaskCreatePinnedToCore(facdefMonitor,"facdeftask",2300,NULL,1,&facdefhandle,1);
  attachInterrupt(POWER_MONITOR_PIN, handlePowerFailureISR, CHANGE);
  startup();
}

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[loop]--------------------STAYS EMPTY--------------------------]
//////////////////////////////////////////////////////////////////////////
void loop() {}
//-----------------------------------------------------------------------]

/* STORAGE

//JsonObject obj = doc.as<JsonObject>();
//for (JsonPair p : obj) {
//auto t = p.key(); // is a JsonString
//const char* s = p.value(); 
//Serial.print(t.c_str());Serial.print("-->");Serial.println(s);
//}
    //Serial.println(cookie);
    //List all parameters (Compatibility)
    //int args = request->args();
    //for(int i=0;i<args;i++){
    //  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    //}

*/