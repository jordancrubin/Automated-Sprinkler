/*
  Sprinkler.h - Header for the Automated Sprinkler Project
  this uses the libraries from the water meter and ball valve
  project.
  https://www.youtube.com/c/jordanrubin6502
  2023 Jordan Rubin.
*/
#include <Ballvalve.h>
#include <Watermeter.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_PCF8574.h>
#include <WiFiClientSecure.h>
#include <Firebase_ESP_Client.h>
#ifndef Sprinkler_h
#define Sprinkler_h

void acctMgr(const char* action,const char* account,const char* val);

 typedef struct {
      char database_url[150];
      char database_secret[150];
  }fbcreds;

// This is the sprinkler class, takes PF575i2caddress, LCDi2caddress, LCDrows, and LCDcols
class SPRINKLERSYSTEM
{
  ////////////// user-accessible "public" interface
  public: 
    SPRINKLERSYSTEM(int,int,int,int);   
    class Zone {
      public:
        Zone(int, const char*);
        int dbKey;
        const char* description;
        int duration;
        int port;
        bool open;
        char* statusMsg;
        const char* thisname;
    };
    fbcreds fb_acct;
    FirebaseData * fbdo;
    FirebaseAuth * auth;
    FirebaseConfig * config;
    FIVEWIREVALVE * thisvalve;
    LiquidCrystal_PCF8574 * lcd; 
    WATERMETER * flowmeter;
    WiFiClientSecure client;
    Zone * storedZones[12];
    bool active;
    char ** statsPtr;
    int backupStartTime;
    bool backupDay;
    bool canceled; 
    double consumption;  
    int days[7] = { 0 };
    const char * enabled;
    bool hasFirebase;
    bool hasRainsensor;
    bool inManual;
    double lastConsumption =-1;
    int lcdaddress;
    int lcdrows;
    int lcdcols;
    bool manualZoneChange;
    int maxZones = 12;
    char measure;
    int pf575address;
    int program;
    int rainSensorGpio;
    bool rainsensorStatus;
    int startTime;
    TaskHandle_t Task1;
    int zoneCount = 0;
    int zoneRemaining;
    void activateSolenoid(int);
    bool addFirebase(const char* database_url, const char* database_secret);
    void addMeter(int,bool,char,long,bool,double,int,bool);
    void addRainSensor(int);
    void addValve(int,int,int,bool); 
    const char* addZone(int intval, char* charval);
    void begin();
    void cancelManual(void);
    void clearEnabled();
    void closeZone(const char*, unsigned long, const char* = NULL);
    void deleteHistory();
    void facdef();
    void factoryDefaultChk();
    bool getDatabaseActive();
    const char * getDescription(const char *);
    String getHistory(int);
    bool getHasRainSensor();
    char getMeasureType();
    int getPort(const char *);
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
    void loadMeter(void);
    bool meterMoved(void); 
    void offsetManual(const char *, tm *);
    void openZone(const char*);
    double readConsumption(); 
    double readMeter(char);/////////////////////// huh
    double readMeter(void);/////////////////////// huh
    bool readRainSensor(void);
    void removeZone(const char*);
    bool runZone(const char*, unsigned long);
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
    int value;
    int getIndex(const char*);
    int getIndex(int);
    void writePf575(uint16_t);
    SemaphoreHandle_t i2cMutex;
  };
#endif