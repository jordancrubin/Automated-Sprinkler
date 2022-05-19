/*
  Sprinkler.cpp - Functions and libraries for the automated sprinkler system as 
  part of the Irrigation Automation Project for ESP32 Using both the 
  5 wirevalve project and the watermeter projects in the design.  More at
  https://www.youtube.com/c/jordanrubin6502
  2022 Jordan Rubin.
*/
#include <Arduino.h>
#include <Sprinkler.h>

//MAIN CONSTRUCTOR ------------------------------------------------------------]
//SPRINKLERSYSTEM SPRINKLERSYSTEM(int statusled)
SPRINKLERSYSTEM::SPRINKLERSYSTEM(int statusled){
  this->LED=statusled;
  this->active=false;
  this->consumption = 0;
  this->enabled = " ";
  pinMode(LED,OUTPUT);
}
// ----------------------------------------------------------------------------]

//ZONE CONSTRUCTOR ------------------------------------------------------------]
SPRINKLERSYSTEM::Zone::Zone(int pin, const char* name){ 
  this->description = "unset";
  this->gpio = pin;
  this->thisname = name;
  pinMode(gpio,OUTPUT);
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addMeter] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::addMeter(int gpioPin, bool usePullup, char measure, long debounceDelay, bool useSpiffs, double increment){
  //WATERMETER WATERMETER(SignalGPIOpin,useInternalPullupResistor,measure[g|l],metervalue)
  flowmeter = new WATERMETER(gpioPin,usePullup,measure,debounceDelay,useSpiffs,increment);
  flowmeter->setMeter(32.2);
  this->measure = measure;
  //Serial.println(flowmeter->initFilesys());
  flowmeter->initFilesys();
}
// ----------------------------------------------------------------------------]

// FUNCTION - [addRainSensor] - [updates rain sendor object attributes---------]
void SPRINKLERSYSTEM::addRainSensor(int gpioPin){
  this->rainSensorGpio = gpioPin;
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
      digitalWrite(storedZones[i]->gpio,HIGH);
      zoneCount++;
      break;
    }
  }
  return 0;
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
    digitalWrite(storedZones[myIndex]->gpio,HIGH);
    storedZones[myIndex]->open = false;
  }
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
int SPRINKLERSYSTEM::getIndex(int gpio){
  int returnIndex = -1;
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      if (storedZones[i]->gpio == gpio){ 
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

// FUNCTION - [getSchedZone] - [Returns zone that should be running right now---]
const char * SPRINKLERSYSTEM::getSchedZone(int elapsed){
Serial.print("ELAPS: ");Serial.println(elapsed);
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
Serial.print(storedZones[i]->thisname); Serial.print(storedZones[i]->duration); Serial.println(storedZones[i]->gpio); 
      int zoneremain = elapsed + storedZones[i]->duration;
 Serial.print("ZR: ");  Serial.println(zoneremain);
      this->zoneRemaining = zoneremain;
      if (zoneremain > 0){ return storedZones[i]->thisname;}
      else {elapsed = elapsed + storedZones[i]->duration;}
Serial.println("---------------");
Serial.print("next with ");Serial.println(elapsed);
    }
  }
  return "-1";
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneCount] - [Returns the number of active zones--------------]
int SPRINKLERSYSTEM::getZoneCount(){
  return this->zoneCount;
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneNames] - [Returns the names of zones in an arrayref-------]
void SPRINKLERSYSTEM::getZoneNames(char (& Array)[12][20]){
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    if(storedZones[i]){
      strncpy(Array[i],storedZones[i]->thisname,20);
    }
  }
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [getZoneRemaining] - [Returns the remaining min of current zone---]
int SPRINKLERSYSTEM::getZoneRemaining(){
  return this->zoneRemaining;
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

// FUNCTION - [isSchedForToday] - [Returns if set to run today------------------]
bool SPRINKLERSYSTEM::isSchedForToday(tm * day){
  int today = day->tm_wday;
  if (today==0){today=6;}
  else {today--;}
  return this->days[today];
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [isTodayComplete] - [Returns if todays prog has completed or not--]
bool SPRINKLERSYSTEM::isTodayComplete(tm * time){
  int i = this->timeToProgStart(time);
  const char *  zoneName = this->getSchedZone(i);
  if(strcmp(zoneName,"-1")==0){
    return 1;
  }
  return 0;
}
// -----------------------------------------------------------------------------]

// FUNCTION - [meterMoved] - [Returns the current version from the Object------]
bool SPRINKLERSYSTEM::meterMoved(void){
  bool status;
  status = flowmeter->updated();
  return status;
}
 // ---------------------------------------------------------------------------] 

// FUNCTION - [offsetManual] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::offsetManual(const char* name, tm * time){
Serial.println("SPR::offsetManual");
  if (!inManual){
Serial.println("Engaging Manual Mode ");
    inManual = true;
    backupStartTime = startTime;
    startTime = time->tm_hour * 100 + time->tm_min;
    int myIndex = this->getIndex(name);
    int offset =0;
    for(int i=myIndex; i> 0; i--){
        Serial.print(i);
        Serial.print(" -> ");
        Serial.println(storedZones[i]->duration);
        offset = offset + storedZones[i]->duration;
    }
    Serial.println(startTime);
    Serial.print(offset);
    startTime = startTime - offset;
  }
else{Serial.println("already in Manual Mode");}

} 
// ---------------------------------------------------------------------------]

// FUNCTION - [cancelManual] - [Returns from Manuak mode to Automatic--------]
void SPRINKLERSYSTEM::cancelManual(){
  inManual = false;
  startTime = backupStartTime;
  Serial.println("Switch to Automatic mode");
}

// FUNCTION - [openZone] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::openZone(const char* name){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {
    this->setConsumption();
    digitalWrite(storedZones[myIndex]->gpio,LOW);
    storedZones[myIndex]->open = true;
  }  
}
// ----------------------------------------------------------------------------]

// FUNCTION - [readconsumption] - [Returns the current version from the Object-]
double SPRINKLERSYSTEM::readConsumption(void){
  return (this->readMeter() - this->consumption);
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [readmeter] - [Returns the current version from the Object--------]
double SPRINKLERSYSTEM::readMeter(void){
  double val;
  val = flowmeter->readOut();
  return val;
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [readmeter] - [Returns the current version from the Object--------]
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

// FUNCTION - [readmeter] - [Returns the current version from the Object--------]
bool SPRINKLERSYSTEM::runZone(const char * zoneName){
Serial.print("VP: ");Serial.println(this->valvePosition());
  if (strcmp(this->valvePosition(),"OPEN")!=0){
    const char * result = this->setValve("OPEN");
    Serial.print("RE: ");Serial.println(result);
  }
  this->closeZone(this->isEnabled());
//store the consumption int he individunal zome
  this->openZone(zoneName);
  this->enabled = zoneName;
  this->active = true;
  return 0;
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [setCanceled] - [Sets the Canceled State of the system------------]
void SPRINKLERSYSTEM::setCanceled(bool val){
  this->canceled = val;
}
 // ----------------------------------------------------------------------------] 

// FUNCTION - [setconsumption] - [Returns the current version from the Object-]
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

// FUNCTION - [setProgram] - [sets the current program A-C equates to 1-3 4 OFF]
void SPRINKLERSYSTEM::setProgram(int programNum){
  const char * prog = "programa";
  int j=3;
  if (programNum ==2){prog = "programb";j=4;}
  if (programNum ==3){prog = "programc";j=5;}
  if (programNum ==4){this->program =programNum; return;}
  File file = SPIFFS.open("/programmes.cnf");
  if (file){
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, file); 
    this->startTime = atoi(doc[prog][0]);
    for(int counter = 0;counter <= 6;counter++) {
      this->days[counter] = doc[prog][counter+1];
    }
    file.close();
  }
  file = SPIFFS.open("/system.cnf");
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

// FUNCTION - [statusLedBlink] - [Controls the Status LED----------------------]
void SPRINKLERSYSTEM::statusLedBlink(int count, int speed){
  if ((count ==1)&&(speed==0)){digitalWrite(LED,HIGH); return;}
  if ((count ==0)&&(speed==0)){digitalWrite(LED,LOW);  return;}
  for (int i = 0; i < count; ++i) {
    delay(speed);
    digitalWrite(LED,HIGH);
    delay(speed);
    digitalWrite(LED,LOW);
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [timeToProgStart()] - [Returns the min until prog begins---------]
int SPRINKLERSYSTEM::timeToProgStart(tm * sttime){
//Serial.print("ST:  ");Serial.println(this->startTime);
//Serial.print("STHRS:  ");Serial.println(this->startTime/100);
//Serial.print("TO HRS:  ");Serial.println(sttime->tm_hour);
//Serial.print("TOT: ");Serial.println(sttime->tm_hour * 100 + sttime->tm_min);
  int res = this->startTime - (sttime->tm_hour * 100 + sttime->tm_min);
//Serial.print("RES: ");Serial.println(res);
  if ( ((this->startTime/100)-(sttime->tm_hour)) >0 ) {res=res-40;}
  return res;
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

// FUNCTION - [zoneInfo] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::zoneInfo(){
  char buf[200];
  //Serial.println("\n\n***ZONE INFO***");
  sprintf(buf,"%-8s %-8s %-18s %-18s\n","INDEX","GPIO","NAME","DESCRIPTION");
  Serial.print(buf);
  for (int i=0; i<sizeof storedZones/sizeof storedZones[0]; i++) {
    buf[0] = '\0';
    char iindex[3]; 
    char igpio[3];
    if(storedZones[i]){
     itoa(i,iindex,10);
     itoa(storedZones[i]->gpio,igpio,10);
     sprintf(buf,"%-8s %-8s %-18s %-18s\n",iindex,igpio,storedZones[i]->thisname,storedZones[i]->description);
     Serial.print(buf);
    }
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [zoneInfo] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::zoneInfo(const char* name){
  int myIndex = this->getIndex(name);
  if (myIndex >=0 && myIndex <=maxZones+1) {
    //Serial.print("\n\n***Zone info for "); Serial.print(name);Serial.println("***");
Serial.println(storedZones[myIndex]->thisname);
    char buf[200];
    char igpio[3];
    const char *enabled = "FALSE";
   // if (storedZones[myIndex]->enabled ==1){enabled = "TRUE";}
    itoa(storedZones[myIndex]->gpio,igpio,10);
    sprintf(buf,"%-11s %-11s\n%-11s %-11s\n%-11s %-11s\n%-11s %-11s\n","GPIO",igpio,"ENABLED",enabled,"STATE","VALUE","DESCIPTION",storedZones[myIndex]->description);
    Serial.print(buf);
  }
  else {
    Serial.println("Invalid Zone name");
  }
}
// ----------------------------------------------------------------------------]

// FUNCTION - [zoneInfo] - [Returns the current version from the Object--------]
void SPRINKLERSYSTEM::info(){
   // Serial.println("\n\n***Information\n");
   // Serial.print("PROGRAM   -> "); Serial.println(this->program);
   // Serial.print("RAIN SENS -> "); Serial.println(this->hasRainsensor);
}
// ----------------------------------------------------------------------------]