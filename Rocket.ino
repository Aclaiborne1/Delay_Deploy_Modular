// MODULE FOR OPERATING SENSORS AND CALCULATING VALUES

// Initialize serial i/o and pins
void initialize()
{
// Setup Serial out for menu
  Serial.begin(9600);

  pinMode(redLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(beepPin, OUTPUT);
  pinMode(relay1Pin, OUTPUT);
  digitalWrite(relay1Pin, LOW);
  
  Timer1.initialize(100000); // set a timer of length 100000 microseconds (or 0.1 sec - or 10Hz => the led will blink 5 times, 5 cycles of on-and-off, per second)
}

// System test
int systemtest()
{
  boolean menu_display;
  int i;
  int error = 0; // initialize error code to zero

// Before anything else, determine whether menu needs to be displayed
  menu_display = retrieve(resetAdd);

// if menu display is indicated (menu mode set or error), set error code
  if (menu_display)
    error = 1; 
  
// menu display not indicated, continue testing

// initialise the accelerometer
  accel.getSensor(&sensor); // attaching the accelerometer

// if there is a problem detecting the ADXL345, go to error routine and stop  
  if (!accel.begin()) 
    error = 2;
  else // accelerometer okay
    accel.setRange(ADXL345_RANGE_16_G); // set range of +- 16G

// Initialise the altimeter
  bmp.getSensor(&sensor);  //attaching the altimeter

// if there is a problem detecting the BMP085, go to error routine and stop
  if(!bmp.begin())
    error = 3;
  
// if there is a problem detecting the FRAM, go to error routine and stop
  if (!fram.begin())
    error = 4;
    
  store(error, resetAdd); // store error code so program enters proper mode on reset
  return(error);
}

// get sea level pressure in millibars, sets global variable
void getseaLevelPressure()
{
// check to see if a value has been entered
  if (retrieve(atmosAdd) == 0)
    store_pressure(1013.9, atmosAdd); // default value, corresponding to 1013.9 millibars
  seaLevelPressure = retrieve_pressure(atmosAdd); 
}

//Takes average of 10 readings, sets global groundlevel in feet as float & stores GL in feet as integer
void getgroundlevel()
{
  int i, intground;
  float floatground, groundacc;
  
  groundacc = 0.0;
  for (i = 0; i < 10; i++)
    groundacc += getaltitude();
  
  floatground = groundacc / 10.0; // gets average value
  groundlevel = 3.2808 * floatground; // converting meters to feet
  
  intground = floatground + 0.5;
  store(intground, groundAdd); // store for posterity
} 

// gets launch temperature, stores deg. Fahrenheit as integer
void gettemperature()
{
  int fahrenheit;
  float temperature;
  
  sensors_event_t event; // get a new sensor event
  
  bmp.getSensor(&sensor); 
  bmp.getEvent(&event);
  
  if (event.pressure)
    bmp.getTemperature(&temperature);
  fahrenheit = (9.0 * temperature / 5.0) + 32.5;
  store(fahrenheit, tempAdd);
}

// returns altitude in meters above sea level
float getaltitude()
{
  float y; 
  sensors_event_t event; // get a new sensor event
  
  bmp.getSensor(&sensor); 
  bmp.getEvent(&event);
  
  if (event.pressure)
  {
    float temperature;
    bmp.getTemperature(&temperature);
    // convert the atmospheric pressure, SLP and temp to altitude
    float Altitude =  (bmp.pressureToAltitude(seaLevelPressure, event.pressure, temperature));
    y = Altitude;
  }
  return(y);
}

//returns height AGL in feet as integer
int getfeet()
{
  int xfeet;
  float height = getaltitude();
  xfeet = ((height * 3.2808) - groundlevel) + 0.5; 
  return(xfeet);
}
    
//returns 100 X actual gees as integer
int getgees()
{
  float xaxis;
  int xgees;
  accel.getSensor(&sensor); // attaching the accelerometer
  accel.getEvent(&event);

  xaxis = event.acceleration.x; // meters per second squared
  xgees = 100.0 * (xaxis / 9.81) + 0.5; // 9.81 meters per second squared is one gee - returning 100 * gees
  return(xgees);
}

// returns apogee in feet as integer
// finds the maximum in i = limit values in FRAM, discards two top values as outliers
int apogee()
{
  int i, temp, mheight, mheight2, mheight3;
  int dataindex = 0;
  mheight = 0; mheight2 = 0; mheight3 = 0;
  for (i=1; i < limit; i++)
  {
    temp = retrieve(altstart + dataindex);
    if (temp > mheight)
      mheight = temp;
    if ((temp < mheight) && (temp > mheight2))
      mheight2 = temp;
    if ((temp < mheight) && (temp < mheight2) && (temp > mheight3))
      mheight3 = temp;
    dataindex += 2; //two bytes to make an integer
  }
  return(mheight3);
}

/* calculates instantaneous velocity over first 75 bytes in FRAM, finds 1st, 2nd and
   3rd maxima, returns 3rd maximum in mph as integer (two higher are outliers) */
int velocity()
{
  int i, mph, address, datapoints;
  float elapsedtime, mspeed, mspeed2, mspeed3, temp, y1, y2, fps, timeint;

  // get the time interval
  delay(6);  // all these delays are needed to give the FRAM recovery time between retrieves
  elapsedtime = retrieve_time(timeAdd); // total elapesed time in flight in seconds
  delay(6);
  datapoints = retrieve(flightcountAdd); // number of intervals in flight
  delay(6);
  timeint = elapsedtime / datapoints; // timeint in seconds per interval

  // initialize variables
  temp = 0.0;
  mspeed = 0.0;
  mspeed2 = 0.0;
  mspeed3 = 0.0;
  
  for (i=0; i <= 75; i++) // upper limit in first 75 bytes in FRAM because max speed will always be in that area
  {
    address = i*2;
    y1 = retrieve(address);
    delay(6);
    y2 = retrieve(address + 10); //measurement 5 data points up (address + 10)
    delay(6);
  
  // now divide the difference by time for five intervals
    temp = (y2 - y1)/(5 * timeint);
    
    if (temp > mspeed)
      mspeed = temp;
 
    if ((temp <= mspeed) && (temp > mspeed2))
      mspeed2 = temp;
    
    if ((temp <= mspeed) && (temp <= mspeed2) && (temp > mspeed3))
      mspeed3 = temp;
  }
  fps = mspeed3;
  mph = fps * 0.6818 + 0.5;
  return(mph);
}

