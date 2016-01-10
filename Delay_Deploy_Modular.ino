// MAIN MODULE, SETS PROGRAM UP, RUNS FLIGHT MODE WHEN resetAdd IS 0

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h> // for ADXL 345 accelerometer
#include <Adafruit_BMP085_U.h> // for altimeter
#include <Adafruit_FRAM_I2C.h> // for FRAM
#include <TimerOne.h> // for interrupt driven flasher
#include "Rocket.h"
#include "Menu.h"
#include "StorageModules.h"

#define beepPin 4
#define redLEDPin 5
#define greenLEDPin 6
#define relay1Pin 7
#define relay2Pin 8
#define UpHeight 700 // floor in feet for upward flight
#define liftOff 15 // 15 feet at datapoint 20 indicates liftoff
#define maxBytes 4000

const int altstart = 0;
const int geesstart = maxBytes/2;
const int limit = maxBytes/4;
const int resetAdd = maxBytes + 16;
const int flightcountAdd = maxBytes + 10;
const int groundAdd = maxBytes + 4;
const int tempAdd = maxBytes + 6;
const int deployAdd = maxBytes + 2;
const int deployHeightAdd = maxBytes + 8;
const int speedAdd = maxBytes + 20;
const int altAdd = maxBytes + 22;
const int atmosAdd = maxBytes + 40;
const int timeAdd = maxBytes + 60;

float seaLevelPressure, groundlevel;

/* Assign a unique ID to all three I2C components at the same time */
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(12345); // for accelerometer
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085); // for altimeter
Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C(); // for FRAM

uint16_t framAddr = 0;
int LEDPin = redLEDPin; // global LEDPin for flashing LED, initialized as red
sensor_t sensor;
sensors_event_t event;

void setup(void) 
{
  int systemerrors, feet, gees, flightCounter, dataindex, countDownHeight, countUpHeight, deployHeight, landedCount;
  boolean goneUp, bang, landed;
  long starttime, elapsedtime;

  Serial.begin(9600);
  initialize(); // initialize i/o and pins
  
  systemerrors = systemtest(); // tests for reset, tests and initializes sensors
  
  if (systemerrors != 0) // if there is a system error, go to error routine and menu
    error(systemerrors);
    
// no system errors, all systems go!

// Siren and turn on solid LED, then fifteen second delay, then siren
  siren();
  flasherOff(); // turn off flasher
  digitalWrite(greenLEDPin, HIGH);  // green LED on solid until ready to launch

  delay(15000);
  siren();
  digitalWrite(greenLEDPin, LOW); // turn off solid green LED
  flasherOn(greenLEDPin); // flashing green LED indicates ready to launch
  
// Get relevant launch data
  gettemperature();
  getseaLevelPressure();
  getgroundlevel();

// INITIALIZE VARIABLES FOR MAIN LOOP
  goneUp = false;
  bang = false;
  landed = false;
  landedCount = 0; // for accuracy in determining landing
  countUpHeight = 0; countDownHeight = 0; // for accuracy in determining when to deploy
  deployHeight = retrieve(deployHeightAdd); // get the deploy height
    if (deployHeight == 0){deployHeight = 500;} // in case the deploy height is not set
  flightCounter = 0;
  starttime = millis();
  
// BEGIN MAIN LOOP
  do
  {
    dataindex = 2 * flightCounter; // each poke takes two bytes
    
    gees = getgees();
    store(gees, geesstart + dataindex); // store the acceleration
    delay(10);

    feet = getfeet();
    store(feet, altstart + dataindex); // store the altitude
    delay(10);
    
 // START SECTION TESTING WHETHER LAUNCH HAS OCCURED
    if (flightCounter == 20) // test to see if launched at datapoint 20
    {
      if (feet < liftOff) // if not at least liftOff feet, then not launched
      {
        flightCounter = 0; // resetting flightCounter because not launched
        getgroundlevel(); // resetting ground level value, pressure may be changing
        starttime = millis(); // resetting start time
      }
      else // launch has occured
      {
        flasherOff(); // turn off flasher
        digitalWrite(greenLEDPin, HIGH); // turn on solid light indicating launch detected
        store(1, resetAdd); //reset will cause program to enter menu mode
      }
    }
// END SECTION TESTING WHETHER LAUNCH HAS OCCURED

//  BEGIN SECTION TESTING FOR DEPLOY HEIGHT
   if (feet > UpHeight) {countUpHeight++;} // above UpHeight?
      else {countUpHeight = 0;}
    if (countUpHeight >= 3) {goneUp = true;} // We must count 3 times above UpHeight feet in a row to be sure
    
 // If we've passed UpHeight feet going up, are we going down below deployHeight?
    if (goneUp && (feet <  deployHeight)) {countDownHeight++;}
    else {countDownHeight = 0;} // We must count 3 times below deployHeight feet in a row to be sure
    if (countDownHeight >= 3) // So now we've gone up past UpHeight and down again below 

    {
      if (!bang)// ignite deploy, write timing (as multiple of time intervals) at deployAdd
      {
        digitalWrite(relay1Pin, HIGH);  // BOOM
        bang = true;
        store(flightCounter, deployAdd);
      }
    }
//  END SECTION TESTING FOR DEPLOY HEIGHT
       
// START SECTION TESTING WHETHER LANDING HAS OCCURED
   if (flightCounter > 20) // launched, now test to see if landed
    {
      if (feet < 2) {landedCount++;}
      if (feet >= 2) {landedCount = 0;}
      if (landedCount == 3) 
      {
        landed = true;
        store(flightCounter, flightcountAdd);
      }
    }
// END SECTION TESTING WHETHER LANDING HAS OCCURED

// INCREMENT FLIGHT COUNTER
    flightCounter++;
  }
  while((flightCounter <= limit) && (!landed)); // loop until landed or all datapoints filled
// END MAIN LOOP

  digitalWrite(relay1Pin, LOW); // turn off relay
  digitalWrite(redLEDPin, HIGH); // both lights on indicating series is complete
  elapsedtime = millis() - starttime;
  store_time(elapsedtime, timeAdd);
  store(apogee(), altAdd); // put maximum height at altAdd
  store(velocity(), speedAdd);  // now put the speed at speedAdd
}  
   
void loop(void) 
{
  siren();
  delay(3000);
  countout(retrieve(altAdd));
  delay(2000);
  countout(retrieve(speedAdd));
  delay(2000);
}
