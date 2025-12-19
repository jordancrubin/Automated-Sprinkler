/*
  Sprinkler.cpp - Functions and libraries for the automated sprinkler system as 
  part of the Irrigation Automation Project for ESP32 Using both the 
  5 wirevalve project and the watermeter projects in the design.  More at
  https://www.youtube.com/c/jordanrubin6502
  2023 Jordan Rubin.
*/
#include <Arduino.h>
#include <Sprinkler.h>
#include <SPIFFS.h>
#include <Wire.h>
#include <time.h>

//MAIN CONSTRUCTOR ------------------------------------------------------------]
//SPRINKLERSYSTEM SPRINKLERSYSTEM(int pf575addr, int lcdi2cval, int lcdrowsval, int lcdcolsval)
SPRINKLERSYSTEM::SPRINKLERSYSTEM(int pf575addr, int lcdi2cval, int lcdrowsval, int lcdcolsval){
  this->pf575address= pf575addr;
  this->active=false;
  this->consumption = 0;
  this->enabled = " ";
  this->lcdaddress = lcdi2cval;
  this->lcdrows = lcdrowsval;
  this->lcdcols = lcdcolsval;
  WiFiClientSecure client;    
  this->program = 4; // Default to OFF
  this->hasFirebase = false;
  this->thisvalve = NULL;
  i2cMutex = xSemaphoreCreateMutex();
}
// ----------------------------------------------------------------------------]

//ZONE CONSTRUCTOR ------------------------------------------------------------]
SPRINKLERSYSTEM::Zone::Zone(int pin, const char* name){ 
  this->description = "unset";
  this->port = pin;
  this->thisname = name;
  this->duration = 0;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [activateSolenoid] - [Opens a solenoid -1 to shut all -----------]
void SPRINKLERSYSTEM::activateSolenoid(int solenoidNum){
  if (solenoidNum == -1){
    this->writePf575(0B1111111111111111); //Set all off
    return;
  }
  int mask   = 0B0000000000000001;
  int offVal = 0B1111111111111111;
  mask = mask << (solenoidNum-1);
  int result = offVal ^ mask;
  this->writePf575(result); 
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addFirebase] - [Adds Firebase connection to the object----------]
bool SPRINKLERSYSTEM::addFirebase(const char* database_url, const char* database_secret){
  strcpy(fb_acct.database_url, database_url);
  strcpy(fb_acct.database_secret, database_secret);  
  fbdo = new FirebaseData();
  fbdo->setBSSLBufferSize(4096, 1024);
  auth = new FirebaseAuth();
  config = new FirebaseConfig();
  config->database_url = fb_acct.database_url;
  config->signer.tokens.legacy_token = fb_acct.database_secret;
  config->timeout.wifiReconnect = 10 * 1000;
  config->timeout.socketConnection = 30 * 1000;
  config->timeout.serverResponse = 10 * 1000;
  config->timeout.rtdbKeepAlive = 45 * 1000;
  config->timeout.rtdbStreamReconnect = 1 * 1000;
  Firebase.begin(config, auth);
  Firebase.reconnectWiFi(true);
  Serial.printf("FBDB init: %s\n", fb_acct.database_url);
  unsigned long start = millis();
  bool connected = false;
  while (millis() - start < 10000) {
    if (Firebase.ready()) {
      if (Firebase.RTDB.get(fbdo, "/test")) {
        Serial.println("FBDB Connected!");
        connected = true;
        break;
      } 
      else {
        Serial.printf("Firebase Error: %s\n", fbdo->errorReason().c_str());
      }
    }
    delay(100);
  }
  if (!connected) Serial.println("FBDB connection failed or timed out.");
  this->hasFirebase = true;
  return true;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addMeter] - [Adds a water meter to the Object-------------------]
void SPRINKLERSYSTEM::addMeter(int gpioPin, bool usePullup, char measure, long debounceDelay, bool useSDfs, double increment,int saveinterval, bool debug){
  flowmeter = new WATERMETER(gpioPin,usePullup,measure,debounceDelay,useSDfs,increment,saveinterval,debug);
  this->measure = measure;
  flowmeter->initFilesys();
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addRainSensor] - [updates rain sendor object attributes---------]
void SPRINKLERSYSTEM::addRainSensor(int gpioPin){
  this->rainSensorGpio = gpioPin;
  pinMode(this->rainSensorGpio, INPUT_PULLUP);
  this->hasRainsensor = true;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addValve] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::addValve(int relaygpio, int opengpio, int closedgpio, bool usepullup){
  thisvalve = new FIVEWIREVALVE(relaygpio,opengpio,closedgpio,usepullup);
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addZone] - [Returns the current version from the Object---------]
const char* SPRINKLERSYSTEM::addZone(int gpio, char* zonename){  
  if (strlen(zonename) > 20){
    return "20Charnamelimit";
  } 
  for(int i=0; i< maxZones; i++){
    if (storedZones[i]){continue;}
    else {
      int myIndex = this->getIndex(zonename);
      if (myIndex >=0 && myIndex <=maxZones+1) {
        return "nameinuse";
      }
      myIndex = this->getIndex(gpio);
      if (myIndex >=0 && myIndex <=maxZones+1) {
        return "gpioinuse";
      }     
      storedZones[i] = new Zone(gpio,zonename);     
      zoneCount++;
      break;
    }
  } 
  return "success";
  return 0;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [begin] - [clears the enabled flagfrom the Object----------------]
void SPRINKLERSYSTEM::begin(){
  lcd = new LiquidCrystal_PCF8574(this->lcdaddress);
  lcd->begin(this->lcdrows,this->lcdcols);
  this->writePf575(0B1111111111111111); //Set all off
  lcd->home();
  lcd->clear();
  lcd->setBacklight(10);
  lcd->print("    *RubinTech*");
  lcd->setCursor(0, 1);
  lcd->print("  Boot  Framework");
  lcd->setCursor(0, 2);  
  if(!startSpiffFs()){
    lcd->print("SPIFFS: Mount err."); 
    return;
  } 
  else {lcd->print("SPIFFS:  Mounted");}
  acctMgr("inspect","admin","0"); // Call acctMgr here to ensure it runs after SPIFFS is mounted
  lcd->setCursor(0, 3); 
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [clearEnabled] - [clears the enabled flag from the Object--------]
void SPRINKLERSYSTEM::clearEnabled(){
  this->enabled = " ";
  this->active = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [closeZone] - [Returns the current version from the Object-------]
void SPRINKLERSYSTEM::closeZone(const char* name, unsigned long now, const char* annotation){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex < maxZones) {
    this->activateSolenoid(-1);
    unsigned long dbKey;
    dbKey = storedZones[myIndex]->dbKey;
    StaticJsonDocument<256> doc;
    char dbstr[11];
    sprintf(dbstr, "%lu", dbKey);    
    doc["edtime"] = now;
    doc["edmeter"] = round(10 * this->readMeter()) / 10;
    doc["duration"] = now-dbKey;
    doc["consumption"] = round(10 * this->readConsumption()) / 10;
    if (annotation) {
      doc["note"] = annotation;
    }
    String serialout;
    serializeJson(doc, serialout);   
    if (this->hasFirebase) {
      String path = "/history/" + String(dbstr);
      FirebaseJson json;
      json.setJsonData(serialout);
      unsigned long startAttempt = millis();
      while (!(Firebase.ready() && Firebase.RTDB.updateNode(fbdo, path, &json))) {
        if (millis() - startAttempt > 15000) {
            Serial.println("[ERROR] Firebase update timed out (closeZone)");
            break;
        }
        delay(1000);
      }
    }
    setMeter(round(10 * this->readMeter()) / 10); 
    storedZones[myIndex]->dbKey = 0;
    storedZones[myIndex]->open = false;
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [deleteHistory] - [Deletes all history from Firebase------------------]
void SPRINKLERSYSTEM::deleteHistory(){
  if (this->hasFirebase) {
    unsigned long startAttempt = millis();
    while (!(Firebase.ready() && Firebase.RTDB.deleteNode(fbdo, "/history"))) {
      if (millis() - startAttempt > 15000) {
          Serial.println("[ERROR] Firebase delete timed out");
          break;
      }
      delay(1000);
    }
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getDatabaseActive] - [returns connection state of Database------]
bool SPRINKLERSYSTEM::getDatabaseActive(){
  if (this->hasFirebase && Firebase.ready()) {
    return true;
  }
  else {
    return false;
  }
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getDescription] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx---------]
const char * SPRINKLERSYSTEM::getDescription(const char* name){
  int myIndex = this->getIndex(name); 
  if (myIndex == -1) {
    return "";
  }
  return storedZones[myIndex]->description;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getHistory] - [Returns history from Firebase---------------------]
String SPRINKLERSYSTEM::getHistory(int days){
  if (this->hasFirebase && Firebase.ready()) {
    time_t now;
    time(&now);
    unsigned long start = now - (86400 * days);
    QueryFilter query;
    query.orderBy("$key");
    query.startAt(String(start));
    if (Firebase.RTDB.getJSON(fbdo, "/history", &query)) {
      return fbdo->payload();
    }
  }
  return "{}";
}
// ----------------------------------------------------------------------------]


// PRIVATE - [getIndex] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx---------]
int SPRINKLERSYSTEM::getIndex(const char* name){
  int returnIndex = -1;
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      if (strcmp(storedZones[i]->thisname,name)==0){ 
        returnIndex = i;
        return returnIndex;
      }
    }
  }
  return returnIndex;
}
// ----------------------------------------------------------------------------]

// PRIVATE - [getIndex] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx---------]
int SPRINKLERSYSTEM::getIndex(int port){
  int returnIndex = -1;
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      if (storedZones[i]->port == port){ 
        returnIndex = i;
        return returnIndex;
      }
    }
  }
  return returnIndex;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getMeasureType] - [Returns the measure type from the Object g/l-]
char SPRINKLERSYSTEM::getMeasureType(){
  return this->measure;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getPort] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx----------------]
int SPRINKLERSYSTEM::getPort(const char* name){
  int myIndex = this->getIndex(name); 
  if (myIndex == -1) {
    return -1;
  }
  return storedZones[myIndex]->port;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getProgram] - [Returns the current program from the Object------]
int SPRINKLERSYSTEM::getProgram(){
  return this->program;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getRainSensor] - [Returns OP state of rain sensor --------------]
bool SPRINKLERSYSTEM::getRainSensor(){
  return this->rainsensorStatus; 
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getHasRainSensor] - [Returns existance rain sensor -------------]
bool SPRINKLERSYSTEM::getHasRainSensor(){
  return this->hasRainsensor; 
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [readRainSensor] - [Returns the rain sensor state ---------------] CONDENSE THIS!!!!!!
bool SPRINKLERSYSTEM::readRainSensor(){
  bool state = digitalRead(this->rainSensorGpio);
  return state; 
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getSchedZone] - [Returns zone that should be running right now--]
const char * SPRINKLERSYSTEM::getSchedZone(int elapsed){
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      int zoneremain = elapsed + storedZones[i]->duration;
      this->zoneRemaining = zoneremain;
      if (zoneremain > 0){return storedZones[i]->thisname;}
      else {elapsed = elapsed + storedZones[i]->duration;}
    }
  }
  return "-1";
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneCount] - [Returns the number of active zones-------------]
int SPRINKLERSYSTEM::getZoneCount(){
  return this->zoneCount;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneNames] - [Returns the names of zones in an arrayref------]
void SPRINKLERSYSTEM::getZoneNames(char (& Array)[12][20]){
  int count = 0;
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      strncpy(Array[count],storedZones[i]->thisname,20);
      count++;
    }
  }
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneRemaining] - [Returns the remaining min of current zone--]
int SPRINKLERSYSTEM::getZoneRemaining(){
  return this->zoneRemaining;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdClearRow] - [Blanks entire row of LCD screen-----------------]
void SPRINKLERSYSTEM::lcdClearRow(int row){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  lcd->setCursor(0, row);
  for(int i=0; i< this->lcdrows; i++){
    lcd->print(" ");
  }
  lcd->setCursor(0, row);
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdDisplaySwVersion] - [Displays software info to LCD-----------]
void SPRINKLERSYSTEM::lcdDisplaySwVersion(const char * version){
  this->lcdPrint("clearrow",4,0,"-RubinTech-");
  this->lcdPrint("clearrow",3,1,"*ESPIRRIGATE*");
  this->lcdPrint("clearrow",1,2,"JORDAN RUBIN 2026");
  this->lcdPrint("clearrow",0,3,"VER. ");
  this->lcdPrintConcat(version);
  delay(8000);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPower] - [Turns the LCD screen ON or OFF---------------------]
void SPRINKLERSYSTEM::lcdPower(bool state){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  if (state ==0){
    lcd->noDisplay();
  }
  else {
    lcd->setBacklight(10);
    lcd->display();
  }
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [prints text to the LCD screen----------------------]
void SPRINKLERSYSTEM::lcdPrint(const char * option, int col, int row, const char* text){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  if(strcmp(option,"clr")==0){
    xSemaphoreGive(i2cMutex);
    lcdClearRow(row);
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
  }
  if(strcmp(option,"cls")==0){lcd->clear();}
  if(strcmp(option,"init")==0){
    lcd->begin(this->lcdrows,this->lcdcols);
    lcd->clear();
    lcd->setCursor(0, 0);
  }
  if(strcmp(option,"concat")!=0){lcd->setCursor(col, row);}
  lcd->print(text);
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [prints text to the LCD screen----------------------]
void SPRINKLERSYSTEM::lcdPrint(const char * option, int col, int row, int num){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  if(strcmp(option,"clr")==0){
    xSemaphoreGive(i2cMutex);
    lcdClearRow(row);
    xSemaphoreTake(i2cMutex, portMAX_DELAY);
  }
  if(strcmp(option,"cls")==0){lcd->clear();}
  if(strcmp(option,"init")==0){
    lcd->begin(this->lcdrows,this->lcdcols);
    lcd->clear();
    lcd->setCursor(0, 0);
  }
  if(strcmp(option,"concat")!=0){lcd->setCursor(col, row);}
  lcd->print(num);
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [continue of print text to the LCD screen from last-]
void SPRINKLERSYSTEM::lcdPrintConcat(const char* text){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  lcd->print(text);
  delay(50);
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [continue of print text to the LCD screen from last-]
void SPRINKLERSYSTEM::lcdPrintConcat(int num){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  lcd->print(num);
  delay(50);
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [removeZone] - [Returns the current version from the Object------]
bool SPRINKLERSYSTEM::isActive(){
  return this->active;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [isCanceled] - [Returns the current canceled state---------------]
bool SPRINKLERSYSTEM::isCanceled(){
  return this->canceled;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [removeZone] - [Returns the current version from the Object------]
const char * SPRINKLERSYSTEM::isEnabled(){
  return this->enabled;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [removeZone] - [Returns the state of inManual--------------------]
bool SPRINKLERSYSTEM::isInManualProgram(){
  return this->inManual;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [removeZone] - [Returns the state of inManual--------------------]
void SPRINKLERSYSTEM::isInManualProgram(bool val){
  if (val == true){
    backupStartTime = startTime;
  }
  this->inManual = val;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [isSchedForToday] - [Returns if set to run today-----------------]
bool SPRINKLERSYSTEM::isSchedForToday(tm * day){
  if (inManual){return 1;}
  int today = day->tm_wday;
  if (today==0){today=6;}
  else {today--;}
  return this->days[today];
}
// ---------------------------------------------------------------------------] 

// FUNCTION - [isTodayComplete] - [Returns if todays prog has completed or not-]
bool SPRINKLERSYSTEM::isTodayComplete(tm * time){
  int i = this->timeToProgStart(time); 
  const char *  zoneName = this->getSchedZone(i);
  if(strcmp(zoneName,"-1")==0){
    return 1;
  }
  return 0;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [loadMeter] - [Loads the last meter value from Firebase----------]
void SPRINKLERSYSTEM::loadMeter(void){
  if (this->hasFirebase && Firebase.ready()) {
    if (Firebase.RTDB.get(fbdo, "/meter")) {
      String type = fbdo->dataType();
      if (type == "int" || type == "integer" || type == "float" || type == "double") {
        double meterVal = fbdo->doubleData();
        Serial.printf("Loaded meter from FBDB: %f\n", meterVal);
        flowmeter->setMeter(meterVal);
      } 
      else {
        Serial.printf("Firebase: /meter is not a number (Type: %s). Payload: %s\n", type.c_str(), fbdo->payload().c_str());
      }
    } 
    else {
      Serial.printf("Firebase: Failed to get /meter. Reason: %s\n", fbdo->errorReason().c_str());
    }
  }
}
// ---------------------------------------------------------------------------] 

// FUNCTION - [meterMoved] - [Returns the current version from the Object------]
bool SPRINKLERSYSTEM::meterMoved(void){
  if (lastConsumption == readConsumption()){return 0;}
  return 1;
}
// ---------------------------------------------------------------------------] 

// FUNCTION - [offsetManual] - [Returns the current version from the Object----]
void SPRINKLERSYSTEM::offsetManual(const char* name, tm * time){
  if (manualZoneChange){
    if (!inManual){
      backupStartTime = startTime;
    }
    manualZoneChange = false;
    startTime = time->tm_hour * 100 + time->tm_min;
    int myIndex = this->getIndex(name);
    if (myIndex == -1) return;
    int offset =0;
    // Sum the duration of all zones *before* the current one.
    for(int i=0; i < myIndex; i++){
      if (storedZones[i]) { // Check if zone exists to prevent crash
        offset = offset + storedZones[i]->duration;
      }
    }
    startTime = startTime - offset;
  }
} 
// ---------------------------------------------------------------------------]

// FUNCTION - [cancelManual] - [Returns from Manuak mode to Automatic---------]
void SPRINKLERSYSTEM::cancelManual(){
  inManual = false;
  startTime = backupStartTime;
}
// ---------------------------------------------------------------------------]

// FUNCTION - [openZone] - [Returns the current version from the Object-------]
void SPRINKLERSYSTEM::openZone(const char* name){
  lastConsumption =-1;
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex < maxZones) {
    this->setConsumption();
    this->activateSolenoid(storedZones[myIndex]->port);
    storedZones[myIndex]->open = true;
  }  
}
// ---------------------------------------------------------------------------]

// FUNCTION - [readconsumption] - [Returns the current vers. from the Object--]
double SPRINKLERSYSTEM::readConsumption(void){
  lastConsumption = this->readMeter() - this->consumption;
  return (lastConsumption);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [readmeter] - [Returns the current version from the Object-------]
double SPRINKLERSYSTEM::readMeter(void){
  double val;
  val = flowmeter->readOut(); 
  return val;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [readmeter] - [Returns the current version from the Object-------]
double SPRINKLERSYSTEM::readMeter(char type){
  double val;
  val = flowmeter->readOut(type); 
  return val;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [removeZone] - [Returns the current version from the Object------]
void SPRINKLERSYSTEM::removeZone(const char* name){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {
    storedZones[myIndex] = NULL; 
    zoneCount--;
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [runZone] - [Conducts the run zone process ----------------------]
bool SPRINKLERSYSTEM::runZone(const char * zoneName, unsigned long now){
  int myIndex = this->getIndex(zoneName);
  if (myIndex == -1) {
    // Safety check to prevent crash if an invalid zone name is ever passed.
    return false;
  }
  StaticJsonDocument<200> doc;
  char nowstr[11];
  sprintf(nowstr, "%lu", now);
  doc["key"] = nowstr;
  doc["port"] = storedZones[myIndex]->port;
  doc["trigger"] = this->inManual;
  doc["stmeter"] = round(10 * this->readMeter()) / 10;
  String serialout;
  serializeJson(doc, serialout);
  if (this->hasFirebase) {
    String path = "/history/" + String(nowstr);
    FirebaseJson json;
    json.setJsonData(serialout);
    unsigned long startAttempt = millis();
    while (!(Firebase.ready() && Firebase.RTDB.setJSON(fbdo, path, &json))) {
      if (millis() - startAttempt > 15000) {
          Serial.println("[ERROR] Firebase update timed out (runZone)");
          break;
      }
      delay(1000);
    }
  }
  if (strcmp(this->valvePosition(),"OPEN")!=0){
    const char * result;
    result = this->setValve("OPEN");
  }
  this->closeZone(this->isEnabled(),now); 
  storedZones[myIndex]->dbKey  = now; //  storedZones[myIndex]->dbKey = nowstr;
  this->openZone(zoneName);
  this->enabled = zoneName;
  this->active = true;
  return 0;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [setCanceled] - [Sets the Canceled State of the system-----------]
void SPRINKLERSYSTEM::setCanceled(bool val){
  this->canceled = val;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [setconsumption] - [Returns the current version from the Object--]
void SPRINKLERSYSTEM::setConsumption(void){
  this->consumption = this->readMeter();
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [setDescription] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx---------]
void SPRINKLERSYSTEM::setDescription(const char* name, const char* description){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {  
    if (strlen(description) > 60){
      return;
    }
    storedZones[myIndex]->description=description;
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [setManualZoneChange] - [set manualzonechange flag---------------]
void SPRINKLERSYSTEM::setManualZoneChange(bool val){
  manualZoneChange = val;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [setMeter] - [set meter value------------------------------------]
void SPRINKLERSYSTEM::setMeter(double val){
  StaticJsonDocument<100> doc;
  doc["items"][0]["key"] = "meter";
  doc["items"][0]["value"] = val;
  String serialout;
  serializeJson(doc, serialout);
  if (this->hasFirebase) {
    unsigned long startAttempt = millis();
    while (!(Firebase.ready() && Firebase.RTDB.setDouble(fbdo, "/meter", val))) {
      if (millis() - startAttempt > 15000) {
          Serial.println("[ERROR] Firebase update timed out (setMeter)");
          break;
      }
      delay(1000);
    }
  }
  flowmeter->setMeter(val);  
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [setProgram] - [sets the current program A-C equates to 1-3 4 OFF]
void SPRINKLERSYSTEM::setProgram(int programNum){
  const char * prog = "programa";
  int j=3;
  if (programNum ==2){prog = "programb";j=4;}
  if (programNum ==3){prog = "programc";j=5;}
  if (programNum ==4){this->program =programNum; return;}
  File progfile = SPIFFS.open("/programmes.cnf","r");
  if (progfile){
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, progfile);
    progfile.close();
    if (error) {
      Serial.printf("Prog deserializeJson() failed: %s\n", error.c_str());
      if (error == DeserializationError::EmptyInput) {
        SPIFFS.remove("/programmes.cnf");
      }
    } else {
      this->startTime = atoi(doc[prog][0]);
      for(int counter = 0;counter <= 6;counter++) {
        this->days[counter] = doc[prog][counter+1];
      }
    }
  }
  File file = SPIFFS.open("/zones.cnf","r");
  if (file){
    DynamicJsonDocument doc(8192);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
      Serial.printf("Sys deserializeJson() failed: %s\n", error.c_str());
    } else {
      JsonArray arr = doc["zones"].as<JsonArray>();
      for (JsonVariant value : arr) {
        JsonArray thiszone = value.as<JsonArray>();
        const char * name = thiszone[1].as<const char*>();
        const char * duration = thiszone[j].as<const char*>();
        storedZones[this->getIndex(name)]->duration = atoi(duration);
      }
    }
  }
  file.close();
  this->program = programNum;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [setRainsensorStatus] - [Sets rain sensor status of the Object---]
void SPRINKLERSYSTEM::setRainsensorStatus(bool value){ 
 this->rainsensorStatus = value;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [setValve] - [Returns the current version from the Object--------]
const char* SPRINKLERSYSTEM::setValve(const char* value){ 
 if (!thisvalve) return "Error: Valve not initialized";
 const char* result = thisvalve->setValvePosition(value);
 return result;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [startSpiffFs] - [Starts file system services--------------------]
bool SPRINKLERSYSTEM::startSpiffFs(){
  if(!SPIFFS.begin(true)){return 0;}
  else {return 1;}
}
// ----------------------------------------------------------------------------]

// FUNCTION - [timeToProgStart()] - [Returns the min until prog begins---------]
int SPRINKLERSYSTEM::timeToProgStart(tm * sttime){
  int res = this->startTime - (sttime->tm_hour * 100 + sttime->tm_min);
  if ( ((this->startTime/100)-(sttime->tm_hour)) >0 ) {res=res-40;}
  int b,c;
  b =(res/100)*60;
  c =(res%100);
  return b+c;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [updateMeter] - [writes meter value to SD card-------------------]
void SPRINKLERSYSTEM::updateMeter(){
  double val = flowmeter->readOut();
  flowmeter->setMeter(val);
  if (this->hasFirebase) {
    unsigned long startAttempt = millis();
    while (!(Firebase.ready() && Firebase.RTDB.setDouble(fbdo, "/meter", val))) {
      if (millis() - startAttempt > 15000) {
          Serial.println("[ERROR] Firebase update timed out (updateMeter)");
          break;
      }
      delay(1000);
    }
  }
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [valveLastDuration] - [Returns the position of the valve---------]
int SPRINKLERSYSTEM::valveLastDuration(char* event){
  return thisvalve->getLastDuration(event);
}
// ----------------------------------------------------------------------------]

// FUNCTION - [valveMaxTravelTime] - [Returns the position of the valve--------]
int SPRINKLERSYSTEM::valveMaxTravelTime(void){
  return thisvalve->getMaxTravelTime();
}
// ----------------------------------------------------------------------------]

// FUNCTION - [valveMaxTravelTime] - [Returns the position of the valve--------]
void SPRINKLERSYSTEM::valveMaxTravelTime(int delay){
  thisvalve->setMaxTraveltime(delay);
}
// ----------------------------------------------------------------------------]

// FUNCTION - [valvePosition] - [Returns the position of the valve-------------]
const char* SPRINKLERSYSTEM::valvePosition(void){
  if (!thisvalve) return "UNKNOWN";
  return thisvalve->getValvePosition();
}
// ----------------------------------------------------------------------------]

// PRIVATE - [writePf575] - [Writes to PF575 GPIO Expander---------------------]
void SPRINKLERSYSTEM::writePf575(uint16_t data){
  xSemaphoreTake(i2cMutex, portMAX_DELAY);
  Wire.beginTransmission(this->pf575address);
  Wire.write(lowByte(data));
  Wire.write(highByte(data));
  Wire.endTransmission();
  xSemaphoreGive(i2cMutex);
}
// ----------------------------------------------------------------------------]