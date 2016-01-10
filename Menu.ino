// MODULE FOR MENU MODE - INVOKED WHEN resetAdd IS NOT 0

// error routine
void error(int errno)
{
  int i;
  
  flasherOff(); // initialize flasher
  flasherOn(redLEDPin); // turn red LED on flashing to indicate menu mode
  
  for (i=1; i<=3; i++)
  {
    tone(beepPin, 200, 1000);
    delay(1100);
  }
  
  delay(3000);
  switch (errno){
    case 1: Serial.println("Menu mode."); break;
    case 2: Serial.println("Accelerometer error."); break;
    case 3: Serial.println("Altimeter error."); break;
    case 4: Serial.println("FRAM error."); break;
    default: break;
  }
  Serial.println();

  // write error number to error byte in FRAM
  store(errno, resetAdd);
  while(1) {menu();}
}

int getinput()
{
  int incomingByte = 0;
  while(Serial.available() <=0) {}
  {
    // read the incoming byte:
    incomingByte = Serial.read();
    return(incomingByte);
  }
}

void menu()
{  
  int i, value, tens, address, deployHeight;
  char x;
  char buffer[6];
  float pressure;
    
  delay(3000); // 10 seconds to set up terminal program on PC
  Serial.println("Altimeter Menu"); Serial.println();
  Serial.println("   1. Dump data");
  Serial.println("   2. Zero out");
  Serial.println("   3. Enter atmospheric pressure");
  Serial.println("   4. Enter deploy altitude");  
  Serial.println("   5. Prepare for launch");
  Serial.println(); Serial.print("Enter option: ");
  
  value = getinput();
  Serial.println(char(value));
  value = int(value) - 48;
  
  switch (value)
  {
    // dump data
    case 1: Serial.println("dump data."); dumpFRAM(); break;
    
    // zero out
    case 2: Serial.println("Zero out.");
            Serial.print("Are you sure (y/n)? "); 
            x = char(getinput());
            Serial.println(x);
            if (x == 'y' || x == 'Y')
            {
              zeroOut();
              store(1, resetAdd);  // remain in menu mode
              Serial.println("zeroed out.");
            }
            break;
    
    // enter atmospheric pressure
    case 3: pressure = retrieve_pressure(atmosAdd);
            Serial.print("Current atmospheric pressure set to "); Serial.print(pressure); Serial.println(" millibars.");
            Serial.println("Input sea level atmospheric pressure (default 1013.9): ");
            while(Serial.available() <=0) {}
            pressure = Serial.parseFloat();
            store_pressure(pressure, atmosAdd);
            Serial.print("Sea level atmospheric pressure = "); Serial.print(retrieve_pressure(atmosAdd)); Serial.println(" millibars.");
            break;
           
    // enter deploy altitude
    case 4: deployHeight = retrieve(deployHeightAdd);
            Serial.print("Deploy height currently set to "); Serial.print(deployHeight); Serial.println(" feet.");
            Serial.println("Input deploy altitude in feet");
            while(Serial.available() <= 0) {} 
            deployHeight = Serial.parseInt();
            Serial.print("Deploy height = "); Serial.print(deployHeight); Serial.println(" feet.");
            
            store(deployHeight, deployHeightAdd);
            break;
            
    // ready for launch
    case 5: Serial.print("Are you sure (y/n)? "); 
              x = char(getinput());
              Serial.println(x);
              if (x == 'y' || x == 'Y')
              {
                store(0, resetAdd); // set switch for flight mode
                Serial.println("Ready for launch.");
                Timer1.detachInterrupt(); // turn off flasher
                while(1)
                {
                  digitalWrite(redLEDPin, HIGH);
                  digitalWrite(greenLEDPin, LOW);
                  delay(200);
                  digitalWrite(redLEDPin, LOW);
                  digitalWrite(greenLEDPin, HIGH);
                  delay(200);
                }
              }
              break;
                
    // default case
     default: break;
  }
  Serial.println();
}

void dumpFRAM()
{
  float elapsedtime, height, gees, initgees, maxgees, timeint, flighttime;
  int datapoints, i, amount, Gmax, Gmaxcount;
  Serial.begin(9600);
  while(Serial.available()!= 0){}
  delay(1000);
  
  elapsedtime = retrieve_time(timeAdd); // elapsed time in seconds
  datapoints = retrieve(flightcountAdd); // number of intervals in flight
    if (datapoints == 0) {datapoints = maxBytes/4;}
  timeint = elapsedtime / datapoints; // timeint in seconds per interval
  Serial.println();
  Serial.print("Time"); Serial.print('\t'); Serial.print("Altitude"); Serial.print('\t'); Serial.println("Acceleration");
  flighttime = 0.0;
  for(i = 0; i < datapoints; i++)
  {
    height = retrieve(altstart + i * 2);
    amount = retrieve(geesstart + i * 2);
    if (abs(amount) > Gmax)
    {
      Gmax = abs(amount);
      Gmaxcount = i;
    }
    if (i == 30)
    {
      initgees = Gmax / 100.0; // highest Gees in first 10 measurements
      Gmax = 0.0; // to get the next highest acceleration
    }
    gees = amount / 100.0; // acceleration is stored in Gs X 100
    while(Serial.available() != 0) {}
    Serial.print(flighttime, 2); Serial.print('\t'); Serial.print(height); Serial.print('\t'); Serial.println(gees);
    delay(5);
    flighttime += timeint;
  }
  
  maxgees = Gmax / 100.0; 
 
  Serial.println();
  Serial.print("Flight time = "); Serial.print(elapsedtime); Serial.print(" seconds; "); 
  Serial.print(" interval = "); Serial.print(timeint, 6);
  Serial.print("; datapoints = "); Serial.println(datapoints);
  Serial.println();
  Serial.print("Sea level atmospheric pressure set to "); Serial.print(retrieve_pressure(atmosAdd)); Serial.println(" millibars.");
  Serial.print("Ground level = "); Serial.print(retrieve(groundAdd)); Serial.println(" feet.");
  Serial.print("Launch temperature = "); Serial.print(retrieve(tempAdd)); Serial.println(" degrees Fahrenheit.");
  Serial.print("Acceleration on launch: "); Serial.print(initgees, 2); Serial.println(" gees.");
  Serial.print("Maximum post-launch acceleration = "); Serial.print(maxgees, 2); Serial.print(" gees at "); Serial.print((Gmaxcount * timeint), 2); Serial.println(" seconds.");
  Serial.print("Maximum altitude = "); Serial.print(retrieve(altAdd)); Serial.println(" feet");
  Serial.print("Maximum velocity = "); Serial.print(retrieve(speedAdd)); Serial.println(" mi/hr");
  amount = retrieve(deployAdd);
  Serial.print("Deploy timing count = "); Serial.print(amount);
  Serial.print(" = t+"); Serial.print(amount * timeint, 2); Serial.println(" seconds");
}

void zeroOut()
{
  int i, j;
  for (i=0; i <= (maxBytes/2) + 90; i++)
  {
    j = i * 2;
    store(0, j);
  }
}


