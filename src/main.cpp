/*
  Rubin Projects Boot Framework and ASYNC web Based Wifi configuration Framework 
  Standardized basic framework for my projects that require no special code nor
  any extra libraries beyond what I regularly use in my projects to put an ESP32
  on the network with an interactive Web based GUI. 
  https://www.youtube.com/c/jordanrubin6502
  2021 Jordan Rubin.
*/
#define WEBSERVPORT 80
#include <Arduino.h>  
#include <WiFi.h>
#include <WiFiAP.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <NTP.h>
#include <Sprinkler.h>
#include <ArduinoJson.h>
#include "mbedtls/md.h"

const char* ssid = "sprinklersystem";    //SSID of the netconfig Access point
const char* deviceName = "sprinkler32";  //Mdns name sprinkler32.local

AsyncWebServer server(WEBSERVPORT);
SPRINKLERSYSTEM sprinklersystem(2,23,5000);

WiFiUDP wifiUdp;
NTP ntp(wifiUdp);
void startup();                         // Pre-declaration for simplicity
String sha1(String payloadStr);

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
  //else if (filename.endsWith(F(".ico"))) return F("image/x-icon");
  //else if (filename.endsWith(F(".xml"))) return F("text/xml");
  //else if (filename.endsWith(F(".pdf"))) return F("application/x-pdf");
  //else if (filename.endsWith(F(".zip"))) return F("application/x-zip");
  //else if (filename.endsWith(F(".gz"))) return F("application/x-gzip");
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
  return;
}
//-----------------------------------------------------------------------]

// FUNCTION - [readConfigFile] - [Returns key pair values from cfg files-------]
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

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[is_authenticated]-------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool is_authenticated(AsyncWebServerRequest *request) {
  Serial.println("In is_authenticated");
  if (request->hasHeader("Cookie")) {
    Serial.print("Cookie: ");
    String cookie = request->header("Cookie");
    Serial.println(cookie);
    char fString[4][70];   
    cookieParser(fString,cookie.c_str());
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
    Serial.print("USER ");    
    Serial.println(cookieuser[1]);
    char filepwd[50];
    readConfigFile(filepwd,"/accounts.cnf",cookieuser[1]);
    Serial.print("PWD ");
    String convFilePwd = filepwd;
    Serial.println(convFilePwd);
    String cookieUser = cookieuser[1];
    String token = sha1(String(cookieUser) + ":" +      
    String(convFilePwd) + ":" + 
    request->client()->remoteIP().toString());
    if (cookie.indexOf("ESPSESSIONID=" + token) != -1) {
      Serial.println("Auth pass");
      return true;
    }
  }
  Serial.println("Auth Fail");
  return false;
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleFileRead]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
bool handleFileRead(AsyncWebServerRequest *request, String path) {
  Serial.print(F("handleFileRead: "));
  Serial.println(path);
  if (!is_authenticated(request)) {
    Serial.println(F("->login"));
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
    }  
    Serial.print("Path-> ");
    Serial.println(path);
    request->send(response);
    return true;
  }
    Serial.println("\tNot Found: " + path);
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
  Serial.println("handleLogin");
  String msg;
  if (request->hasHeader("Cookie")) {
    // Print cookies
    //Serial.print("Found cookie: ");
    String cookie = request->header("Cookie");
    Serial.println(cookie);
    //List all parameters (Compatibility)
    //int args = request->args();
    //for(int i=0;i<args;i++){
    //  Serial.printf("ARG[%s]: %s\n", request->argName(i).c_str(), request->arg(i).c_str());
    //}
  }
  String user = request->arg("username");
  if (user.length()<1){
    msg = "Provide username/password! try again.";
    AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
    response->addHeader("Location", "/login.html?msg=" + msg);
    response->addHeader("Cache-Control", "no-cache");
    request->send(response);
    return;
  }

  else {
    //Serial.print("Found param: ");
//condense here
    char filepwd[50];
    readConfigFile(filepwd,"/accounts.cnf",user.c_str());
    String convFilePwd = filepwd;
    Serial.print("PWD is: ");     Serial.println(filepwd);
    if (convFilePwd == request->arg("password")){
      AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
      response->addHeader("Location", "/");
      response->addHeader("Cache-Control", "no-cache");
      String token = sha1(request->arg("username") + ":" + convFilePwd + ":" + request->client()->remoteIP().toString());
      //Serial.print("Token: ");
      Serial.println(token);
      response->addHeader("Set-Cookie", "ESPSESSIONID=" + token);
      response->addHeader("Set-Cookie", "USER=" + request->arg("username"));
      request->send(response);
      //Serial.println("Login Success");
      return;
    }
    msg = "Wrong username/password! try again.";
    AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
    response->addHeader("Location", "/login.html?msg=" + msg);
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
  Serial.println("Disco");
  AsyncWebServerResponse *response = request->beginResponse(301); //Sends 301 redirect
  response->addHeader("Location", "/login.html?msg=User disconnected");
  response->addHeader("Cache-Control", "no-cache");
  response->addHeader("Set-Cookie", "ESPSESSIONID=0");
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
    char result[30];
    readConfigFile(result,"/testresult.cnf","CONN");       
    Serial.print(F("CONN -> ")); Serial.println(result);
    request->send(200, "text/html", result);
    if (strcmp(result,"SUCCESS") == 0){
      Serial.println(F("Reboot to normal op."));
      ESP.restart();
    }
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleUpdateConfig]-----------------------------------------]----------------
//////////////////////////////////////////////////////////////////////////
void handleUpdateConfig(AsyncWebServerRequest *request) {
  String s = "{\"Hello\":\"WORLD\"}";
 
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, request->arg("testval").c_str());

    const char* password = doc["pass"];
    Serial.println(password);
    sprinklersystem.writeConfigFile("blabla","/accounts.cnf",password);

  request->send(200, "text/html", s);
  return;
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
// External rest end point (out of authentication)
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", HTTP_GET, handleLogout);
  server.on("/updateConfig", HTTP_POST, handleUpdateConfig);
  server.onNotFound([](AsyncWebServerRequest *request) {               // If the client requests any URI
    if (!handleFileRead(request, request->url())){                  // send it if it exists
      handleNotFound(request); // otherwise, respond with a 404 (Not Found) error
      Serial.println(request->url());
    }
  });
  //Serial.println(F("Set cache"));
  // Serve a file with no cache so every tile It's downloaded
  server.serveStatic("/configuration.json", SPIFFS, "/configuration.json", "no-cache, no-store, must-revalidate");
  // Server all other page with long cache so browser chaching they
  server.serveStatic("/", SPIFFS, "/", "max-age=31536000");
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[stringToarray]----------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void stringToarray (char * convstr, String input) {
   int str_len = input.length() + 1; 
   input.toCharArray(convstr, str_len);   
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiConnect]------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiConnect(AsyncWebServerRequest *request) {  
  AsyncWebParameter* p = request->getParam(0);
  char ssid[50];
  char password[75];
  stringToarray(ssid, p->value());
  p = request->getParam(1);
  stringToarray(password, p->value());
  File testfile = SPIFFS.open("/testnetwork.cnf","w");
  if (!testfile){
      Serial.print(F("open testnetwork file err"));
      return;  
  }
  testfile.print("{\"SSID\":\""); testfile.print(ssid);
  testfile.print("\",\"PASSWORD\":\""); testfile.print(password); testfile.print("\"}"); 
  testfile.close();
  request->send(200, "text/html", "RCVD"); 
  startup();
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-FUNCTION-[handleWifiList]---------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void handleWifiList(AsyncWebServerRequest *request) {
  int n = WiFi.scanNetworks(); 
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
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP:   ");
  Serial.println(myIP);
  if (!MDNS.begin(deviceName)){
    Serial.print(F("Error starting mDNS"));
  }
  if (SPIFFS.exists("/testnetwork.cnf")){
    char ssid[50]; 
    readConfigFile(ssid,"/testnetwork.cnf","SSID");         
    Serial.print(F("Attempt with SSID -> ")); Serial.println(ssid);
    char key[50];
    readConfigFile(key,"/testnetwork.cnf","PASSWORD");
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    File resultfile = SPIFFS.open("/testresult.cnf","w");
    if (!resultfile){
      Serial.print("Open testresult file err");
      return;  
    }    
    WiFi.begin(ssid,key);
    int i =0; 
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      sprinklersystem.statusLedBlink(1,100);
      i++;
      if (i==15){
        Serial.print("\nWifi Conn. Failed, Restarting\n");
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
  Serial.println(WiFi.localIP());
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
    Serial.print("    SSID -> "); Serial.println(ssid);
    char key[50];
    readConfigFile(key,"/network.cnf","PASSWORD");  
    if(SPIFFS.exists("/testresult.cnf")){SPIFFS.remove("/testresult.cnf");}
    WiFi.begin(ssid,key);
    int i =0;
    Serial.print("Waiting.");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      sprinklersystem.statusLedBlink(1,100);
      i++;
      if (i==15){
        Serial.print("\nNETWORK: Wifi Conn. Failed, Restart\n");
        delay(100);     
        startup();        
    }
  }
  sprinklersystem.statusLedBlink(1,0);
  if (!MDNS.begin(deviceName)){
    Serial.print("\nmDNS:    Error Starting.");
  }
  else {
    Serial.print("\nmDNS:    Listed as ");Serial.print(deviceName); Serial.println(".local");
  }  
  Serial.print("NETWORK: Connected at "); Serial.println(WiFi.localIP());
  ntp.begin();
  Serial.print("NTP:     ");
  Serial.print(ntp.formattedTime("%d. %B %Y  ")); // dd. Mmm yyyy
  Serial.println(ntp.formattedTime("%T")); // dd. Mmm yyyy
}
//-----------------------------------------------------------------------]

// FUNCTION - [acctMgr] - [Initializes and handles user accounts---------------]
void acctMgr(const char* action,const char* account,const char* val){
 if(!SPIFFS.exists("/accounts.cnf")){
    File accountfile = SPIFFS.open("/accounts.cnf","w");
    if (!accountfile){
      Serial.print("wri err!");
      return;  
    }
    if ((action == "inspect")&&(account =="admin")&&(val=="0")){
      accountfile.println("{\"admin\":\"password\"}"); 
      accountfile.close();
      Serial.println("USERS:   Created account file with default admin");
    }  
    return;
 }
  Serial.println("USERS:   Account file exists");
}
// ----------------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[startup]------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void startup(){
  sprinklersystem.factoryDefaultChk();
  acctMgr("inspect","admin","0");
  //sprinklersystem.acctMgr("inspect","admin","0");
  if (SPIFFS.exists("/network.cnf")){
     Serial.println("NETWORK: Already configured, Loading......");
     loadNetwork();
  }
  else {
     Serial.println("NETWORK: Setting up for initial config.....");   
     configNetwork();    
  }
  server.on("/getWifiList",  HTTP_GET, handleWifiList);
  server.on("/connectwWifi", HTTP_POST, handleWifiConnect); 
  server.on("/checkStatus",  HTTP_GET, handleCheckStatus); 
  serverRouting();
  server.begin();
  Serial.print("WEBSVR:  Running on port ");Serial.println(WEBSERVPORT);
}
//-----------------------------------------------------------------------]

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[setup]--------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void setup() {
  Serial.begin(115200);
  Serial.println("\n\n***Rubin Projects Boot Framework***\n   technocoma.blogspot.com 2021\n");
  if(!sprinklersystem.startSpiffFs()){
      Serial.println("SPIFFS: Mount err.");
      return;
  } 
  else {Serial.println("SPIFFS:  Mounted");}
  //SPIFFS.remove("/accounts.cnf");
  startup();
}

//////////////////////////////////////////////////////////////////////////
//-SYSTEM-[loop]---------------------------------------------------------]
//////////////////////////////////////////////////////////////////////////
void loop() {
  sprinklersystem.factoryDefaultChk();
  delay(5000);
}
//-----------------------------------------------------------------------]