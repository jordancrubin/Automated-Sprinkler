/*
  Sprinkler.h - Header for the Automated Sprinkler Project
  this uses the libraries from the water meter and ball valve
  project.
  https://www.youtube.com/c/jordanrubin6502
  2021 Jordan Rubin.
*/
#include <Ballvalve.h>
#include <Watermeter.h>
#include <SPIFFS.h>
// ensure this library description is only included once
#ifndef Sprinkler_h
#define Sprinkler_h

// library interface description
// This is a sprinklersystem. There are many like it, but this one is mine.
class SPRINKLERSYSTEM
{
  ////////////// user-accessible "public" interface
  public:
  SPRINKLERSYSTEM(int,int,int);   
class Zone {
  public:
    Zone(int, const char*);
    const char* thisname;
    int gpio;
    const char* description;
    char* statusMsg;
    bool open;
    bool enabled;
};
    FIVEWIREVALVE * thisvalve;
    WATERMETER * flowmeter;
    Zone * storedZones[30];
    int maxZones = 30;
    int LED;
    int FACDEFPIN;
    int FACDEFDELAY;
    void addMeter(int,bool,char,long,bool,double,bool);
    void addValve(int,int,int,bool); 
    void addZone(int intval, const char* charval);
    void closeZone(const char*);
    void factoryDefaultChk();
    bool meterMoved(void); 
    void openZone(const char*); 
    double readMeter(char);
    double readMeter(void);
    void removeZone(const char*);
    void setDescription(const char*, const char*);
    const char* setValve(char*);
    bool startSpiffFs();
    void statusLedBlink(int,int);
    int valveLastDuration(char*);
    int valveMaxTravelTime(void);
    void valveMaxTravelTime(int);
    const char* valvePosition(void);
    void writeConfigFile(char*, const char*, const char*);
    void zoneInfo();
    void zoneInfo(const char*);

  ////////////// library-accessible "private" interface
  private:
    int value;
    int getIndex(const char*);
    int getIndex(int);
  };
#endif