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
#include <ArduinoJson.h>
// ensure this library description is only included once
#ifndef Sprinkler_h
#define Sprinkler_h

// library interface description
// int statusledgpio, int hardresetgpio, int facdefdelayms
// This is the onboard LED gpio, the gpio assigned as the factory reset button
// and how many seconds for factory default with the button in milliseconds.
// Usage of the onboard ESP32 led is generally 2, default reset gpio for this
// project is 23, and 5 second wait is 5000 (2,23,5000)
class SPRINKLERSYSTEM
{
  ////////////// user-accessible "public" interface
  public: 
  SPRINKLERSYSTEM(int);   
class Zone {
  public:
    Zone(int, const char*);
    const char* description;
    int duration;
    int gpio;
    bool open;
    char* statusMsg;
    const char* thisname;
};
    FIVEWIREVALVE * thisvalve;
    WATERMETER * flowmeter;
    Zone * storedZones[12];
    bool active;
    bool canceled; 
    double consumption;  
    int days[7] = { 0 };
    const char * enabled;
    bool hasRainsensor;
    int LED;
    bool inManual;
    int maxZones = 12;
    char measure;
    int program;
    int rainSensorGpio;
    int startTime;
    int backupStartTime;
    int zoneCount = 0;
    int zoneRemaining;
    void addMeter(int,bool,char,long,bool,double);
    void addRainSensor(int);
    void addValve(int,int,int,bool); 
    const char* addZone(int intval, char* charval);
    void cancelManual(void);
    void clearEnabled();
    void closeZone(const char*);
    void facdef();
    void factoryDefaultChk();
    char getMeasureType();
    int getProgram();
    const char * getSchedZone(int);
    int getZoneCount();
    void getZoneNames(char (& Array)[12][20]);
    int getZoneRemaining();
    void info();
    bool isActive();
    bool isCanceled();
    const char* isEnabled(void);
    bool isSchedForToday(tm *);
    bool isTodayComplete(tm *);
    bool meterMoved(void); 
    void offsetManual(const char *, tm *);
    void openZone(const char*);
    double readConsumption(); 
    double readMeter(char);/////////////////////////// huh
    double readMeter(void);///////////////////////////// huh
    void removeZone(const char*);
    bool runZone(const char*);
    void setCanceled(bool);
    void setConsumption(void);
    void setDescription(const char*, const char*);
    void setProgram(int);

    const char* setValve(const char*);
    
    bool startSpiffFs();
    void statusLedBlink(int,int);
    int timeToProgStart(tm *);
    int valveLastDuration(char*);
    int valveMaxTravelTime(void);
    void valveMaxTravelTime(int);
    const char* valvePosition(void);
    void zoneInfo();
    void zoneInfo(const char*);

  ////////////// library-accessible "private" interface
  private:
    int getIndex(const char*);
    int getIndex(int);
    int value;
  };
#endif