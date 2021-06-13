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
// int statusledgpio, int hardresetgpio, int facdefdelayms
// This is the onboard LED gpio, the gpio assigned as the factory reset button
// and how many seconds for factory default with the button in milliseconds.
// Usage of the onboard ESP32 led is generally 2, default reset gpio for this
// project is 23, and 5 second wait is 5000 (2,23,5000)
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
    Zone * storedZones[12];
    int maxZones = 12;
    int LED;
    int FACDEFPIN;
    int FACDEFDELAY;
    void addMeter(int,bool,char,long,bool,double);
    void addValve(int,int,int,bool); 
    void addZone(int intval, char* charval);
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
    void zoneInfo();
    void zoneInfo(const char*);

  ////////////// library-accessible "private" interface
  private:
    int value;
    int getIndex(const char*);
    int getIndex(int);
  };
#endif