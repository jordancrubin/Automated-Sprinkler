/*
  Sprinkler.cpp - Functions and libraries for the automated sprinkler system as 
  part of the Irrigation Automation Project for ESP32 Using both the 
  5 wirevalve project and the watermeter projects in the design.  More at
  https://www.youtube.com/c/jordanrubin6502
  2022 Jordan Rubin.
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
Serial.println(result, BIN);
  this->writePf575(result); 
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addDetabase] - [Adds Detabase connection to the object----------]
bool SPRINKLERSYSTEM::addDetabase(const char* id, const char* name, const char* apikey){
  detaObj = new DetaBaseObject(this->client, id, name, apikey, true);
  this->hasDetabase = true;
  const char * testid = detaObj->getDetaID();
  if (strcmp(testid,id)==0){
    this->detabaseconnect = true;
      printResult(detaObj->query("{\"query\":[{\"age?lt\": 10}]}"));
    return 1;
  }
  return 0;
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

// FUNCTION - [clearEnabled] - [clears the enabled flagfrom the Object---------]
void SPRINKLERSYSTEM::clearEnabled(){
  this->enabled = " ";
  this->active = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [closeZone] - [Returns the current version from the Object-------]
void SPRINKLERSYSTEM::closeZone(const char* name){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {
    this->activateSolenoid(-1);
    storedZones[myIndex]->open = false;
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [getDatabaseActive] - [returns connection state of Database------]
bool SPRINKLERSYSTEM::getDatabaseActive(){
  return this->detabaseconnect;
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
  while (this->lcdlock){delay (lcdLockDelay);}
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
  this->lcdPrint("clearrow",1,2,"JORDAN RUBIN 2022");
  this->lcdPrint("clearrow",0,3,"VER. ");
  this->lcdPrintConcat(version);
  delay(8000);
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
  while (this->lcdlock){delay (lcdLockDelay);}
  this->lcdlock = true;
  if(strcmp(option,"clearrow")==0){
    this->lcdlock = false;
    lcdClearRow(row);
    this->lcdlock = true;
  }
  if(strcmp(option,"clearscreen")==0){lcd->clear();}
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
  while (this->lcdlock){delay (lcdLockDelay);}
  this->lcdlock = true;
  if(strcmp(option,"clearrow")==0){
    this->lcdlock = false;
    lcdClearRow(row);
    this->lcdlock = true;
  }
   if(strcmp(option,"clearscreen")==0){lcd->clear();}
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
  while (this->lcdlock){delay (lcdLockDelay);}
  this->lcdlock = true;
  lcd->print(text);
  delay(50);
  this->lcdlock = false;
}
// ----------------------------------------------------------------------------] 

// FUNCTION - [lcdPrint] - [continue of print text to the LCD screen from last-]
void SPRINKLERSYSTEM::lcdPrintConcat(int num){
  while (this->lcdlock){delay (50);}
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

// FUNCTION - [meterMoved] - [Returns the current version from the Object------]
bool SPRINKLERSYSTEM::meterMoved(void){
  bool status;
  status = flowmeter->updated();
  return status;
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
  return (this->readMeter() - this->consumption);
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

// FUNCTION - [readmeter] - [Returns the current version from the Object-------]
bool SPRINKLERSYSTEM::runZone(const char * zoneName){
  if (strcmp(this->valvePosition(),"OPEN")!=0){
    const char * result = this->setValve("OPEN");   ///////////////////CHECK THIS!!!!!!!!!!!!
  }
  this->closeZone(this->isEnabled());
  //store the consumption in the individual zone
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
