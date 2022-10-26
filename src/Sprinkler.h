/*
  Sprinkler.h - Header for the Automated Sprinkler Project
  this uses the libraries from the water meter and ball valve
  project.
  https://www.youtube.com/c/jordanrubin6502
  2022 Jordan Rubin.
*/
#include <Ballvalve.h>
#include <Watermeter.h>
#include <SPIFFS.h>
//#include <SD.h> // check for removal
//#include <FS.h> //check for removal
#include <ArduinoJson.h>
#include <detaBaseArduinoESP32.h>
#include <LiquidCrystal_PCF8574.h>
// ensure this library description is only included once
#ifndef Sprinkler_h
#define Sprinkler_h

// library interface description
class SPRINKLERSYSTEM
{
  ////////////// user-accessible "public" interface
  public: 
  SPRINKLERSYSTEM(int , int, int, int );   
  //SPRINKLERSYSTEM(int); 
class Zone {
  public:
    Zone(int, const char*);
    const char* description;
    int duration;
    int port;
    bool open;
    char* statusMsg;
    const char* thisname;
};
    DetaBaseObject * detaObj;
    FIVEWIREVALVE * thisvalve;
    LiquidCrystal_PCF8574 * lcd; 
    WATERMETER * flowmeter;
    WiFiClientSecure client;
    Zone * storedZones[12];
    bool active;
    int backupStartTime;
    bool backupDay;
    bool canceled; 
    double consumption;  
    int days[7] = { 0 };
    bool detabaseconnect;
    const char * enabled;
    bool hasDetabase;
    bool hasRainsensor;
    bool inManual;
    int lcdaddress;
    int lcdrows;
    int lcdcols;
    bool lcdlock;
    int lcdLockDelay = 20;
    bool manualZoneChange;
    int maxZones = 12;
    char measure;
    int pf575address;
    int program;
    int rainSensorGpio;
    bool rainsensorStatus;
    int startTime;
    int zoneCount = 0;
    int zoneRemaining;
    void activateSolenoid(int);
    bool addDetabase(const char* id,const char* name,const char* apikey);
    void addMeter(int,bool,char,long,bool,double,int,bool);
    void addRainSensor(int);
    void addValve(int,int,int,bool); 
    const char* addZone(int intval, char* charval);
    void begin();
    void cancelManual(void);
    void clearEnabled();
    void closeZone(const char*);
    void facdef();
    void factoryDefaultChk();
    bool getDatabaseActive();
    const char * getDescription(const char *);
    bool getHasRainSensor();
    char getMeasureType();
    int getProgram();
    bool getRainSensor();
    const char * getSchedZone(int);
    int getZoneCount();
    void getZoneNames(char (& Array)[12][20]);
    int getZoneRemaining();
    bool isActive();
    bool isCanceled();
    const char* isEnabled(void);
    bool isInManualProgram();
    void isInManualProgram(bool);
    bool isSchedForToday(tm *);
    bool isTodayComplete(tm *);
    void lcdClearRow(int);
    void lcdDisplaySwVersion(const char *);
    void lcdPower(bool);
    void lcdPrint(const char *, int, int, const char *);
    void lcdPrint(const char *, int, int, int);
    void lcdPrintConcat(const char *);
    void lcdPrintConcat(int);
    bool meterMoved(void); 
    void offsetManual(const char *, tm *);
    void openZone(const char*);
    double readConsumption(); 
    double readMeter(char);/////////////////////// huh
    double readMeter(void);/////////////////////// huh
    bool readRainSensor(void);
    void removeZone(const char*);
    bool runZone(const char*);
    void setCanceled(bool);
    void setConsumption(void);
    void setDescription(const char*, const char*);
    void setMeter(double);
    void setProgram(int);
    void setRainsensorStatus(bool);
    void setManualZoneChange(bool);
    const char* setValve(const char*);
    bool startSpiffFs();
    int timeToProgStart(tm *);
    void updateMeter();
    int valveLastDuration(char*);
    int valveMaxTravelTime(void);
    void valveMaxTravelTime(int);
    const char* valvePosition(void);

  ////////////// library-accessible "private" interface
  private:
    int getIndex(const char*);
    int getIndex(int);
    int value;
    void writePf575(uint16_t);
  };
#endif