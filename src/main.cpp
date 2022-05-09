/*
  Rubin Projects Boot Framework and ASYNC web Based Wifi configuration Framework 
  Standardized basic framework for my projects that require no special code nor
  any extra libraries beyond what I regularly use in my projects to put an ESP32
  on the network with an interactive Web based GUI. 
  https://www.youtube.com/c/jordanrubin6502
  2022 Jordan Rubin.
*/
#define ESP32LED 2
#define FACDEFDELAY 5000
#define FACDEFPIN 23
#define LCDCOLS 4
#define LCDI2C 0x27
#define LCDROWS 20
#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 4
#define ROTARY_ENCODER_BUTTON_PIN 33
#define ROTARY_ENCODER_STEPS 4
#define ROTARY_ENCODER_VCC_PIN -1
#define WEBSERVPORT 80
#include "AiEsp32RotaryEncoder.h"
#include <Arduino.h>
#include <ArduinoJson.h>
//#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LiquidCrystal_PCF8574.h> 
#include "mbedtls/md.h"
#include <SD.h>
#include <Sprinkler.h>
#include "time.h"
#include <Wire.h> 
#include <WiFi.h>
#include <WiFiAP.h>

const char* deviceName = "sprinkler32";  //Mdns name sprinkler32.local
int displayprogram = 0;
double lastTimepoll = -60000;
const char* ntpServer = "pool.ntp.org";
const char* ssid = "sprinklernet";    //SSID of the netconfig Access point
struct tm timeinfo;
bool displayLock;
int lastEncval;
int menulevel = 0;
int selectlevel = 0;
int programMenu = -1;
bool inManualProgram = false;
char selectedProgram[20];
const char * menu[3][5] = {
  {"CANCEL PROGRAMME","START PROGRAMME","REBOOT UNIT","SW VERSION","EXIT MENU"},
  {"BACk","Ctest1","Ctest2","Ctest3","Ctest4"} 
};

AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);
AsyncWebServer server(WEBSERVPORT);
LiquidCrystal_PCF8574 lcd(LCDI2C); 
SPRINKLERSYSTEM sprinklersystem(ESP32LED);
void startup();                         // Pre-declaration for simplicity
String sha1(String payloadStr);

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[clearLcrRow]------Simple LCD Clearrow function--------------]
//////////////////////////////////////////////////////////////////////////
void clearLcdRow(int row) {
  lcd.setCursor(0, row);
  for(int i=0; i< LCDROWS; i++){
    lcd.print(" ");
  }
  lcd.setCursor(0, row);
}  
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[getContentType]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
String getContentType(String filename) {
  if (filename.endsWith(F(".htm"))) return F("text/html");
  else if (filename.endsWith(F(".html"))) return F("text/html");
  else if (filename.endsWith(F(".css"))) return F("text/css");
  else if (filename.endsWith(F(".js"))) return F("application/javascript");
  else if (filename.endsWith(F(".json"))) return F("application/json");
  else if (filename.endsWith(F(".png"))) return F("image/png");
  else if (filename.endsWith(F(".gif"))) return F("image/gif");
  else if (filename.endsWith(F(".jpg"))) return F("image/jpeg");
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

// FUNCTION - [readConfigFile] - [Returns key pair values from cfg files-]
void readConfigFile(char* value, const char* filename, const char* parameter){
  File file = SPIFFS.open(filename);
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
// ----------------------------------------------------------------------------] 

// FUNCTION - [writeConfigFile] - [Adds or updates key pair values in cfg files-]
void writeConfigFile(const char* value, const char* filename, const char* parameter){ 
  File file = SPIFFS.open(filename);
  if (file){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file);
    file.close();
    doc[value] = parameter;
    File resultfile = SPIFFS.open(filename,"w");
    serializeJson(doc, resultfile); 
  } 
  file.close();
}
// ----------------------------------------------------------------------------]

// FUNCTION - [loadConfig] - [Returns the current version from the Object------]
void loadConfig(){
  clearLcdRow(3);
  clearLcdRow(1); 
  lcd.print("LOADCFG:");
  clearLcdRow(2);
  File file = SPIFFS.open("/system.cnf");
  lcd.print("/system.cnf");
///////////////
//while (file.available()) {
//  Serial.write(file.read());
//}
//file.close();
//file = SPIFFS.open("/system.cnf");
/////////////  
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, file);
  file.close();
  const char* mtrio = doc["mtrio"];
  const char* meas = doc["meas"];
  const char* deb = doc["mtrdeb"];
  const char* inc = doc["mtrinc"];
  sprinklersystem.addMeter(atoi(mtrio),1,meas[0],atoi(deb),1,atof(inc));
  const char* valopio = doc["valopio"];
  const char* valclio = doc["valclio"];
  const char* valrlyio = doc["valrlyio"];
  const char* valrainio = doc["rainsens"];
  sprinklersystem.addValve(atoi(valrlyio),atoi(valopio),atoi(valclio),1);
  sprinklersystem.addRainSensor(atoi(valrainio));
  JsonArray arr = doc["zones"].as<JsonArray>();
  for (JsonVariant value : arr) {
    JsonArray thiszone = value;
    const char * gpio = thiszone.getElement(0);
    const char * name = thiszone.getElement(1);
    const char * desc = thiszone.getElement(2);
    char* newname = strdup(name);
    char* newdesc = strdup(desc);
    sprinklersystem.addZone(atoi(gpio),newname);
    sprinklersystem.setDescription(name,newdesc);
  }
    if (doc.containsKey("program")){
    const char* program = doc["program"];
    sprinklersystem.setProgram(atoi(program));
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
    if(SPIFFS.exists(pathWithGz)) {                         // If there's a compressed version available
      path += F(".gz");                                     // Use the compressed version
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
          request->redirect("/programme.html");
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
  if (request->arg("event") == "progchange") { 
    const char * val = request->arg("value").c_str();   
    writeConfigFile("program","/system.cnf",val);   
    sprinklersystem.setProgram(atoi(val));
    request->send(200, "text/html", "{\"s\":\"0\"}");
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
  int i = sprinklersystem.timeToProgStart(&timeinfo);
  if(sprinklersystem.getProgram()==4){state="0";} //programme off
  else if(!sprinklersystem.isSchedForToday(&timeinfo)){state ="1";} //Idle, not today
  else if(sprinklersystem.isCanceled()){state ="2";} // Manual cancellation
  else if(i>0){
      state ="3";
      info = i;
  } //countdown to start
  else if(sprinklersystem.isTodayComplete(&timeinfo)){state="4";} //completed for today 
  else {
      state="5";
      const char * zoneName = sprinklersystem.getSchedZone(i);
      int zr = sprinklersystem.getZoneRemaining();
      Serial.print("I is "); Serial.println(zoneName);
      zname = zoneName;
      info = zr;
    }//currently running

  String S = "{\"state\":\""+state+"\",\"info\":\""+info+"\",\"zone\":\""+zname+"\"}";
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
  server.on("/login",         HTTP_POST, handleLogin);
  server.on("/logout",        HTTP_GET, handleLogout);
  server.on("/updateConfig",  HTTP_POST, handleUpdateConfig);
  server.on("/getConf",       HTTP_POST, handleGetconf);
  server.on("/getProg",       HTTP_POST, handleGetprogram);
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
  lcd.clear();
  lcd.print("    \nRubinTech\n");
  clearLcdRow(1);
  lcd.print("WIFI: sprinklernet");
  clearLcdRow(2);
  lcd.print("Configure at");
  clearLcdRow(3);
  lcd.print(deviceName);lcd.print(".local");
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
      sprinklersystem.statusLedBlink(1,100);
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
    clearLcdRow(2);
    lcd.print("Connecting");
    clearLcdRow(3);
    char key[50];
    readConfigFile(key,"/network.cnf","PASSWORD");  
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    WiFi.begin(ssid,key);
    int i =0;
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      lcd.print(".");
      sprinklersystem.statusLedBlink(1,100);
      i++;
      if (i==15){
        clearLcdRow(2);
        lcd.print("Connect Fail");
        clearLcdRow(3);
        lcd.print("Restart....");
        delay(100);     
        startup();        
    }
  }
  sprinklersystem.statusLedBlink(1,0);
  clearLcdRow(3);
  if (!MDNS.begin(deviceName)){
    lcd.print("mDNS: Error Starting.");
  }
  else {
    lcd.print(deviceName); lcd.print(".local");
  }
  clearLcdRow(2);
  lcd.print(WiFi.localIP());
  delay(1000);
  clearLcdRow(1);
  lcd.print("NTP: ");
  configTime(0, 0, ntpServer);
  if (getLocalTime(&timeinfo)){
    lcd.print("OK"); 
  }
  else {
    lcd.print("FAILED");
    delay(1000);
    ESP.restart();
  }
}
//-----------------------------------------------------------------------]

// FUNCTION - [acctMgr] - [Initializes and handles user accounts---------------]
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
 //}   
 //if(!SPIFFS.exists("/perms.cnf")){
 //   File permfile = SPIFFS.open("/perms.cnf","w");
 //   if (!permfile){
 //     Serial.print("wri err!");
 //     return;  
 //   }
 //   if ((action == "inspect")&&(account =="admin")&&(val=="0")){
 //     permfile.println("{\"admin\":\"admin\"}"); 
 //     permfile.close();
 //   }  
// }
  //  Serial.println("USERS:   Created account file with default admin");
    return;
 }
 // Serial.println("USERS:   Account file exists");
}
// ----------------------------------------------------------------------------]

/////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayheader]---------------------------------------------]
/////////////////////////////////////////////////////////////////////////
void displayHeader() {
  lastTimepoll = millis();
  getLocalTime(&timeinfo);
  if (!displayLock){
    lcd.begin(LCDROWS, LCDCOLS);
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(&timeinfo, "%A  %H:%M");
    lcd.setCursor(0,1);
    lcd.print("Program ");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayMeter]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayMeter() {
  if (!displayLock){
    if(sprinklersystem.isActive()){
      char mtype = sprinklersystem.getMeasureType();
      double val = sprinklersystem.readConsumption(); 
      delay(350);
      lcd.setCursor(0,3);
      lcd.print(val);
      lcd.print(mtype);
      lcd.setCursor(0,3);
      delay(350);
    }
  } 
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[startZone]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool startZone(const char * zone) {
Serial.print("startZone() check for ");  Serial.println(zone);
Serial.print("isenabled ");  Serial.println(sprinklersystem.isEnabled());
  if (strcmp(sprinklersystem.isEnabled(),zone)==0){
    return 0;   
  }
Serial.print("run runzone");
  sprinklersystem.runZone(zone);
  return 0;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[checkMenu]--------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool checkMenu(){
  if ( (programMenu >=0) && (!inManualProgram) ){
    int lastOption = sprinklersystem.getZoneCount();
    if(programMenu == lastOption){
      programMenu = -1;
      menulevel =-1;
      selectlevel=-1;
    }
    //Send off program at selectedProgram
Serial.print("SPX:   ");Serial.println(selectedProgram);




  if (strcmp(selectedProgram,"BACK")==0){
    Serial.println("CLICKED BACK");
    selectlevel=-1;
    return 0;
  }
    displayLock = false;
    lastTimepoll = -60000;
    menulevel =0;
    selectlevel =0;
    inManualProgram = true;
    startZone(selectedProgram);
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"BACK")==0){
    Serial.println("CLICKED BACK");
    selectlevel--;
    return 0;
  }
  if (strcmp(menu[menulevel][selectlevel],"REBOOT UNIT")==0){
    ESP.restart();
  }
  if (strcmp(menu[menulevel][selectlevel],"EXIT MENU")==0){
    displayLock = false;
    lastTimepoll = -60000;
    menulevel =0;
    selectlevel =0;
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"CANCEL PROGRAMME")==0){
    sprinklersystem.setCanceled(true);
    displayLock = false;
    inManualProgram = false;
    sprinklersystem.cancelManual();
    lastTimepoll = -60000;
    menulevel = 0;
    selectlevel = 0;
    return 1;
  }
  if (strcmp(menu[menulevel][selectlevel],"START PROGRAMME")==0){
    programMenu = 0;
Serial.println("PGMENU TRUE");
    return 0;
  }
// {"CANCEL PROGRAMME","START PROGRAMME","REBOOT UNIT","SW VERSION","EXIT MENU"},
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
  clearLcdRow(2);
  strncpy(selectedProgram,nameList[programMenu],20);
  lcd.print(nameList[programMenu]);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayMeter]-----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loadMenu(int level){
  if ((programMenu >=0) && (!inManualProgram)){
    displayEachProgram();
    return;
  }
  clearLcdRow(2);
  if (selectlevel>4){selectlevel=4;}
  if (selectlevel<0){selectlevel=0;}   //////////////!!!!!!!!!!!!!!!!!!!!!!!!!!//////////////// change this to appropriate max value
Serial.print("L: "); Serial.print(level); Serial.print("  SL: "); Serial.print(selectlevel); Serial.println(menu[level][selectlevel]);
  lcd.print(menu[level][selectlevel]);
} 
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayCountdown]-------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayCountdown(int i) {
  if (!displayLock){
    lcd.setCursor(0,2);
    lcd.print("Begins in ");
    lcd.setCursor(0,3);
    if (i > 60){
      int hours = i/100;
      int minutes = i%100;
      lcd.print(hours);
      lcd.print("HR ");
      lcd.print(minutes);
    }
    else{
      lcd.print(i);
    }
    lcd.print(" minutes");
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[displayProgram]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void displayProgram() {
  int testprog = sprinklersystem.getProgram();
Serial.print(testprog);
  if (!displayLock){
      if(testprog==1){lcd.print("[A] ");}
      if(testprog==2){lcd.print("[B] ");}
      if(testprog==3){lcd.print("[C] ");}
      if(testprog==4){lcd.print("OFF");}
  }
      displayprogram = testprog;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[closeZones]-------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void closeZones() {
Serial.println("CLOSE ZONES");
Serial.print(sprinklersystem.setValve("CLOSED"));
  sprinklersystem.closeZone(sprinklersystem.isEnabled());
  sprinklersystem.clearEnabled();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleManualsched]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleManualsched(){
Serial.print("handleManualsched()");
Serial.println(selectedProgram);
  getLocalTime(&timeinfo);
//Set the Start time to now or the offset based on the zone start
  sprinklersystem.offsetManual(selectedProgram, &timeinfo);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-TASK-[everyMinute]----------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void everyMinute(void * parameter){
  for (;;){
    delay(700); //LCD NEEDED
  //  Serial.println(sprinklersystem.valvePosition());
    if ((millis()-lastTimepoll) >= 30000){ //60000 NORM.
      displayHeader();
      displayProgram();
Serial.print("IMP bool value");Serial.println(inManualProgram);
      if (inManualProgram){
        handleManualsched();        
      }
        if((!sprinklersystem.isSchedForToday(&timeinfo)) ||
           sprinklersystem.getProgram()==4){
            if (!displayLock){
              lcd.setCursor(0,3);
              lcd.print("        IDLE");


//check close valve




            }
        }
      else if(sprinklersystem.isCanceled()){
          if (!displayLock){
            lcd.setCursor(0,3);
            lcd.print("      CANCELED");
            }
            if(sprinklersystem.isActive()){closeZones();}
            if(sprinklersystem.isTodayComplete(&timeinfo)){
              sprinklersystem.setCanceled(false);
            }
      }
      else {
        int i = sprinklersystem.timeToProgStart(&timeinfo);
        if (i > 0){
          displayCountdown(i);        
        }
        else { //Todays Activity Arrived or passed
         const char *  zoneName = sprinklersystem.getSchedZone(i);
Serial.print("ActiveZone ");    Serial.println(zoneName);   
         if(sprinklersystem.isTodayComplete(&timeinfo)){
    if (inManualProgram){
Serial.println("endManualStuff");
    inManualProgram=false;
    sprinklersystem.cancelManual();
    }           
            if (!displayLock){
              lcd.setCursor(0,2);
              lcd.print("Today's Schedule");
              lcd.setCursor(0,3);
              lcd.print("has completed..");
            }
            if(sprinklersystem.isActive()){closeZones();}
          }
          else {
            if(!displayLock){
              int zr = sprinklersystem.getZoneRemaining();
              //PLACEHOLDER TO KICKOFF THE RUN TASK AND HALT THIS ONE
              lcd.print(zr); lcd.print("min");
              lcd.setCursor(0,2);
              lcd.print(zoneName);
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
        if (encval < lastEncval){
        if (!inManualProgram){if (programMenu >0){programMenu--;}}
          selectlevel--;
        }
        else {
          if (!inManualProgram){if (programMenu >=0){programMenu++;}}
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
Serial.println("Here is me");
Serial.print("ML ");
Serial.println(menulevel);
Serial.print("SL ");
Serial.println(selectlevel);
        menulevel = selectlevel+1;
Serial.println("Here is me 2");
Serial.print("ML ");
Serial.println(menulevel);
Serial.print("SL ");
Serial.println(selectlevel);
        }
      if (selectlevel < 0){selectlevel =0;}
      if (!displayLock){
        displayLock=true;
        lcd.begin(LCDROWS, LCDCOLS);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("       *MENU*");
      }       
        loadMenu(menulevel);
	  }
  }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[startup]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void startup(){
  acctMgr("inspect","admin","0");
  if (SPIFFS.exists("/network.cnf")){
     lcd.print("NETWK: Loading");
     loadNetwork();
  }
  else {
     lcd.print("NETWK: No Config");   
     configNetwork();    
  }
  server.on("/getWifiList",  HTTP_GET, handleWifiList);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect); 
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus); 
  serverRouting();
  server.begin();
  delay(2000);
  if (SPIFFS.exists("/system.cnf")){
    loadConfig();
    delay(500);
    clearLcdRow(3);
    if (!SPIFFS.exists("/programmes.cnf")){
      lcd.print("CONFIG INCOMPLETE");
    } 
    else {
      lcd.print("PROCESS COMPLETE");
      delay(1000);
      TaskHandle_t everyminutehandle = NULL;
      TaskHandle_t rotaryLoophandle  = NULL;
      xTaskCreatePinnedToCore(everyMinute,"everyminutetask",2400,NULL,1,&everyminutehandle,1);
      xTaskCreatePinnedToCore(rotaryLoop,"rotaryLooptask",1800,NULL,1,&rotaryLoophandle,1);
      sprinklersystem.info();
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
        clearLcdRow(1);
        clearLcdRow(3);
        clearLcdRow(2);  
        lcd.print("Defaulting unit!");
        SPIFFS.remove("/network.cnf");
        SPIFFS.remove("/testresult.cnf");
        SPIFFS.remove("/testnetwork.cnf");
        SPIFFS.remove("configuration.json");
        SPIFFS.remove("/accounts.cnf");
        SPIFFS.remove("/system.cnf");
        SPIFFS.remove("/programmes.cnf");
        clearLcdRow(1);
        clearLcdRow(2);
        clearLcdRow(3);
        lcd.print("DEFAULTED!!!!");
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
  int error;
  Serial.begin(115200);
  TaskHandle_t facdefhandle = NULL;
  pinMode(FACDEFPIN, INPUT_PULLUP);
//lcd.init();
  lcd.begin(LCDROWS, LCDCOLS);
  rotaryEncoder.setBoundaries(0,10,false);
  rotaryEncoder.begin();
  rotaryEncoder.disableAcceleration();
	rotaryEncoder.setup(readEncoderISR);
  xTaskCreatePinnedToCore(facdefMonitor,"facdeftask",1800,NULL,1,&facdefhandle,1);
  lcd.home();
  lcd.clear();
  lcd.setBacklight(10);
  lcd.print("    \nRubinTech\n");
  lcd.setCursor(0, 1);
  lcd.print("  Boot  Framework");
  lcd.setCursor(0, 2);
  if(!sprinklersystem.startSpiffFs()){
      lcd.print("SPIFFS: Mount err.");
      return;
  } 
  else {lcd.print("SPIFFS:  Mounted");}
  //SPIFFS.remove("/accounts.cnf");
  lcd.setCursor(0, 3);
  startup();
}

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[loop]--------------------STAYS EMPTY--------------------------]
//////////////////////////////////////////////////////////////////////////
void loop() { }
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





