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
}
// ----------------------------------------------------------------------------]

//ZONE CONSTRUCTOR ------------------------------------------------------------]
SPRINKLERSYSTEM::Zone::Zone(int pin, const char* name){ 
  this->description = "unset";
  this->port = pin;
  this->thisname = name;
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

// FUNCTION - [addDetabase] - [Adds Detabase connection to the object----------] 
bool SPRINKLERSYSTEM::addDetabase(const char* id, const char* name, const char* apikey){
  char* _id = new char[strlen(id)+1];         strcpy(_id, id);
  char* _name = new char[strlen(name)+1];     strcpy(_name, name);
  char* _apikey = new char[strlen(apikey)+1]; strcpy(_apikey, apikey);
  detaObj = new DetaBaseObject(client, _id, _name, _apikey, true);
  this->hasDetabase = true;
  const char * testid = detaObj->getDetaID();
  strcpy(acct.id,_id);
  strcpy(acct.name,_name);
  strcpy(acct.apikey,_apikey);
  if (strcmp(testid,id)==0){
    this->detabaseconnect = true;
    xTaskCreatePinnedToCore(this->databaseQtask,"dbQtask",5000,this,1,&Task1,1); 
    return 1;
  }
  return 0;
 }
// ----------------------------------------------------------------------------]

// FUNCTION - [getDataAcct] - [Pull login info for detabase--------------------]
void SPRINKLERSYSTEM::getDetaAcct(dbcreds * r){
 strcpy(r->name,acct.name);
 strcpy(r->id,acct.id); 
 strcpy(r->apikey,acct.apikey);
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
  return 0;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [begin] - [clears the enabled flagfrom the Object----------------]
void SPRINKLERSYSTEM::begin(){
  Wire.begin();
  this->writePf575(0B1111111111111111); //Set all off
  lcd = new LiquidCrystal_PCF8574(this->lcdaddress);
  lcd->begin(this->lcdrows,this->lcdcols);
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
void SPRINKLERSYSTEM::closeZone(const char* name, unsigned long now){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {
    this->activateSolenoid(-1);
    int dbKey;
    dbKey = storedZones[myIndex]->dbKey;
    StaticJsonDocument<200> doc;
    char dbstr[11];
    sprintf(dbstr, "%d", dbKey);    
    doc["set"]["edtime"] = now;
    doc["set"]["edmeter"] = round(10 * this->readMeter()) / 10;
    doc["set"]["duration"] = now-dbKey;
    doc["set"]["consumption"] = round(10 * this->readConsumption()) / 10;
    String serialout;
    serializeJson(doc, serialout);   
    database("UPDATE",serialout.c_str(),dbstr);
    setMeter(round(10 * this->readMeter()) / 10); 
    storedZones[myIndex]->dbKey = 0;
    storedZones[myIndex]->open = false;
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [database] - [database interactor -------------------------------]
int SPRINKLERSYSTEM::database(const char * action, const char * value){ 
  int freeslot =-1;
  for (int i = 0; i < 5; i++) {
    if(dbq[i].timestamp ==0){
      freeslot =i;  
      break;
    }
  }
  if (freeslot <0){
    return 1;
  }
  dbq[freeslot].timestamp = millis();
  strcpy(dbq[freeslot].Action, action);
  strcpy(dbq[freeslot].Payload, value);
  return 0;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [database] - [database interactor -------------------------------]
int SPRINKLERSYSTEM::database(const char * action, const char * value, const char * value2){ 
  int freeslot =-1;
  for (int i = 0; i < 5; i++) {
    if(dbq[i].timestamp ==0){
      freeslot =i;  
      break;
    }
  }
  if ( (strcmp(action,"QUERY")==0) || (strcmp(action,"SELECT")==0) ){
    for (int i = 0; i < 5; i++) {
      if(strcmp(dbq[i].name,value)==0){ 
        freeslot =i;  
        break; 
     }
    }
  }
  if (freeslot <0){
    return 1;
  }
  dbq[freeslot].timestamp = millis();
  strcpy(dbq[freeslot].Action, action);
  strcpy(dbq[freeslot].Payload, value);
  strcpy(dbq[freeslot].Payload2, value2);
  if ( (strcmp(action,"QUERY")==0) || (strcmp(action,"SELECT")==0) ){
    strcpy(dbq[freeslot].name, value);
  }
  return 0;
}

// TASK - [databaseQtask] - [Processes Database Queue -------------------------]
void SPRINKLERSYSTEM::databaseQtask(void *pvParameters){
  SPRINKLERSYSTEM *taskThis = (SPRINKLERSYSTEM *) pvParameters;
  for(;;){
  bool found =0;     
  int slot;
  int arrayval;
  for (int i = 0; i < 5; i++) {    
    if(taskThis->dbq[i].timestamp ==0){
Serial.print(i);Serial.print("] ");Serial.print(taskThis->dbq[i].timestamp); Serial.println(" NULL");
      continue;
    }
    if(taskThis->dbq[i].timestamp ==-1){
Serial.print(i); Serial.print(" Active-> ");Serial.println(taskThis->dbq[i].name);
      continue;
    }

Serial.print(i); Serial.print(" FOUND ");Serial.println(taskThis->dbq[i].timestamp);   
    found = true;
    slot = taskThis->dbq[i].timestamp;
    break;
  }
  if (!found){Serial.println("Idle"); sleep(5);continue;}  
Serial.print("slot: "); Serial.println(slot);
  for(int j=0; j<5; j++){
    unsigned long ts = taskThis->dbq[j].timestamp;
    if (ts == 0){  
Serial.print(j);  Serial.println(" *ZERO*");
      continue;
    }
Serial.print(j);Serial.print(" -> ["); Serial.print(slot);   Serial.print(" -> ["); Serial.print(ts);Serial.println("]");
    if(slot >= ts ){     
      slot = ts;
      Serial.print(j);  Serial.print(" ++> "); Serial.println(taskThis->dbq[j].timestamp);
      arrayval =j; 
    }
  }
      result myresult;
      if(strcmp(taskThis->dbq[arrayval].Action,"PUT")==0){
        myresult = taskThis->detaObj->putObject(taskThis->dbq[arrayval].Payload);
      }
      if(strcmp(taskThis->dbq[arrayval].Action,"SELECT")==0){
        myresult = taskThis->detaObj->getObject(taskThis->dbq[arrayval].Payload2);
        taskThis->dbq[arrayval].result = myresult.reply;
        taskThis->dbq[arrayval].timestamp = -1;
 //Serial.println(myresult.reply);       
      }
      if(strcmp(taskThis->dbq[arrayval].Action,"DELETE")==0){
        myresult = taskThis->detaObj->deleteObject(taskThis->dbq[arrayval].Payload);
      }
       if(strcmp(taskThis->dbq[arrayval].Action,"INSERT")==0){
        myresult = taskThis->detaObj->insertObject(taskThis->dbq[arrayval].Payload);
      }
       if(strcmp(taskThis->dbq[arrayval].Action,"UPDATE")==0){
        myresult = taskThis->detaObj->updateObject(taskThis->dbq[arrayval].Payload,taskThis->dbq[arrayval].Payload2);
//Serial.println(myresult.reply);
      }
       if(strcmp(taskThis->dbq[arrayval].Action,"QUERY")==0){
        myresult = taskThis->detaObj->query(taskThis->dbq[arrayval].Payload2);
        taskThis->dbq[arrayval].result = myresult.reply;
//Serial.println(taskThis->dbq[arrayval].result);      
        taskThis->dbq[arrayval].timestamp =-1;
      }
   Serial.print(taskThis->dbq[arrayval].Action);   
  if ( (strcmp(taskThis->dbq[arrayval].Action,"QUERY")!=0) && (strcmp(taskThis->dbq[arrayval].Action,"SELECT")!=0)){
Serial.print("Deleted");  
    taskThis->dbq[arrayval].timestamp =0;
  }
    sleep(2);
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getDatabaseActive] - [returns connection state of Database------]
bool SPRINKLERSYSTEM::getDatabaseActive(){
  return this->detabaseconnect;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getDatabaseQuery] - [returns Selected Query from the queue------]
String SPRINKLERSYSTEM::getDatabaseQuery(const char * queryname){
  int value;
  for (int i = 0; i < 5; i++) {
    if(strcmp(dbq[i].name,queryname)==0){ 
      while (dbq[i].timestamp > 0){
        sleep(2);
      }
        strcpy(dbq[i].name, "");
        dbq[i].timestamp =0;
        value =i; 
        break;   
   }
  }
  return dbq[value].result;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [getDescription] - [xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx---------]
const char * SPRINKLERSYSTEM::getDescription(const char* name){
  int myIndex = this->getIndex(name); 
  return storedZones[myIndex]->description;
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
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      strncpy(Array[i],storedZones[i]->thisname,20);
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
  lcdLockCheck();
  lcd->setCursor(0, row);
  for(int i=0; i< this->lcdrows; i++){
    lcd->print(" ");
  }
  lcd->setCursor(0, row);
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdDisplaySwVersion] - [Displays software info to LCD-----------]
void SPRINKLERSYSTEM::lcdDisplaySwVersion(const char * version){
  this->lcdPrint("clearrow",4,0,"-RubinTech-");
  this->lcdPrint("clearrow",3,1,"*ESPIRRIGATE*");
  this->lcdPrint("clearrow",1,2,"JORDAN RUBIN 2023");
  this->lcdPrint("clearrow",0,3,"VER. ");
  this->lcdPrintConcat(version);
  delay(8000);
}
// ----------------------------------------------------------------------------] 

// PRIVATE - [lcdLockCheck] - [Conducts LCD Lock Unlock timing functions-------]
void SPRINKLERSYSTEM::lcdLockCheck(){
  int time = millis();
  while (this->lcdlock){ 
    delay(20);
    if((millis()-time) > lcdlockmax){
      return;      
    }
  }
}
// ----------------------------------------------------------------------------]   

// FUNCTION - [lcdPower] - [Turns the LCD screen ON or OFF---------------------]
void SPRINKLERSYSTEM::lcdPower(bool state){
  if (state ==0){
    lcd->noDisplay();
  }
  else {
    lcd->setBacklight(10);
    lcd->display();
  }
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [prints text to the LCD screen----------------------]
void SPRINKLERSYSTEM::lcdPrint(const char * option, int col, int row, const char* text){
  lcdLockCheck();
  this->lcdlock = true;
  if(strcmp(option,"clr")==0){
    this->lcdlock = false;
    lcdClearRow(row);
    this->lcdlock = true;
  }
  if(strcmp(option,"cls")==0){lcd->clear();}
  if(strcmp(option,"init")==0){
    lcd->begin(this->lcdrows,this->lcdcols);
    lcd->clear();
    lcd->setCursor(0, 0);
  }
  if(strcmp(option,"concat")!=0){lcd->setCursor(col, row);}
  lcd->print(text);
  this->lcdlock = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [prints text to the LCD screen----------------------]
void SPRINKLERSYSTEM::lcdPrint(const char * option, int col, int row, int num){
  lcdLockCheck();
  this->lcdlock = true;
  if(strcmp(option,"clr")==0){
    this->lcdlock = false;
    lcdClearRow(row);
    this->lcdlock = true;
  }
   if(strcmp(option,"cls")==0){lcd->clear();}
   if(strcmp(option,"init")==0){
    lcd->begin(this->lcdrows,this->lcdcols);
    lcd->clear();
    lcd->setCursor(0, 0);
  }
  if(strcmp(option,"concat")!=0){lcd->setCursor(col, row);}
  lcd->print(num);
  this->lcdlock = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [continue of print text to the LCD screen from last-]
void SPRINKLERSYSTEM::lcdPrintConcat(const char* text){
  lcdLockCheck();
  this->lcdlock = true;
  lcd->print(text);
  delay(50);
  this->lcdlock = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [continue of print text to the LCD screen from last-]
void SPRINKLERSYSTEM::lcdPrintConcat(int num){
  lcdLockCheck();
  this->lcdlock = true;
  lcd->print(num);
  delay(50);
  this->lcdlock = false;
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

// FUNCTION - [loadMeter] - [Returns the current version from the Object-------]
void SPRINKLERSYSTEM::loadMeter(void){
  database("SELECT","loadmeter","meter");
  String returnVal = getDatabaseQuery("loadmeter");
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, returnVal); 
  double meterval = doc["value"];
  flowmeter->setMeter(meterval); 
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
    int offset =0;
    for(int i=myIndex; i> 0; i--){
        offset = offset + storedZones[i]->duration;
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
  if (myIndex >=0 && myIndex <=maxZones+1) {
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
  StaticJsonDocument<200> doc;
  char nowstr[11];
  sprintf(nowstr, "%d", now);
  doc["items"][0]["key"] = nowstr;
  doc["items"][0]["port"] = storedZones[myIndex]->port;
  doc["items"][0]["trigger"] = manualZoneChange;
  doc["items"][0]["stmeter"] = round(10 * this->readMeter()) / 10;
  String serialout;
  serializeJson(doc, serialout);
Serial.print(serialout);
  database("PUT",serialout.c_str());
  if (strcmp(this->valvePosition(),"OPEN")!=0){
    const char * result = this->setValve("OPEN");   //////////ACCEPTABLE WARNING
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
  database("PUT",serialout.c_str());
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
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, progfile); 
    this->startTime = atoi(doc[prog][0]);
    for(int counter = 0;counter <= 6;counter++) {
      this->days[counter] = doc[prog][counter+1];
    }
    progfile.close();
  }
  File file = SPIFFS.open("/system.cnf","r");
  if (file){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file); 
    JsonArray arr = doc["zones"].as<JsonArray>();
    for (JsonVariant value : arr) {
      JsonArray thiszone = value;
      const char * name = thiszone.getElement(1);
      const char * duration = thiszone.getElement(j);
      storedZones[this->getIndex(name)]->duration = atoi(duration);
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
 const char* result = thisvalve->setValvePosition(value);
 return result;
}
// ----------------------------------------------------------------------------]

// FUNCTION - [startSpiffFs] - [Starts file system services--------------------]
bool SPRINKLERSYSTEM::startSpiffFs(){
  if(!SPIFFS.begin()){return 0;}
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
  StaticJsonDocument<200> doc; 
  doc["set"]["meter"] = val;
  String serialout;
  serializeJson(doc, serialout);
  database("UPDATE",serialout.c_str(),"meter");
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
  return thisvalve->getValvePosition();
}
// ----------------------------------------------------------------------------]

// PRIVATE - [writePf575] - [Writes to PF575 GPIO Expander---------------------]
void SPRINKLERSYSTEM::writePf575(uint16_t data){
  Wire.beginTransmission(this->pf575address);
  Wire.write(lowByte(data));
  Wire.write(highByte(data));
  Wire.endTransmission();
}
// ----------------------------------------------------------------------------]