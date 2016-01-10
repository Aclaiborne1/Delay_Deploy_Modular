// MODULE FOR STORING, RETRIEVING, BEEPING AND FLASHING

// general siren routine
void siren()
{
  delay(1000);
  tone(beepPin, 3000, 3000);
  delay(1000); 
}

// beeps j times
void beeptimes(int j)
{
  int i;
  for (i=1; i<=j; i++)
  {
    tone(beepPin, 3000, 200);
    delay(500);
  }
}
    
// sends out beep-coded-decimal for c
void countout(int c)
{
  int x;
  boolean First = true;
  int digit=10000;
  while (digit > 0)
  {
    x = c / digit;
  
    if ((!First) && (x ==0)) { beeptimes(10); delay(1000);}
    else {  if (x > 0) {beeptimes(x); delay(1000);} }
    if ((First) && (x > 0)){ First = false; }
    c = c % digit;
    digit = digit / 10;
  }
  return;
}

// store and retrieve routines
void store(int z, int bytecounter)
// stores integer in two bytes of FRAM starting at bytecounter
{
  fram.begin();
  fram.write8(bytecounter, highByte(z));
  fram.write8(bytecounter + 1, lowByte(z));
}

int retrieve(int bytecounter)
// retrieves integer from two bytes of FRAM starting at bytecounter
{
  fram.begin();
  byte high = fram.read8(bytecounter);
  byte low = fram.read8(bytecounter + 1);
  int z = (high << 8) + low;
  return(z);
}

// receives milliseconds, stores time in seconds * 100 at address
void store_time(long duration, int address)
{
  int sec100;
  
  sec100 = duration / 10;
  store(sec100, address);
}

// returns time as float seconds from address
float retrieve_time(int address)
{
  float value;
  value = retrieve(address) / 100.0;
  return(value);
}

// receives pressure as float, stores pressure as millibars * 10 at address
void store_pressure(float pressure, int address)
{
  int millibars10;
  millibars10 = pressure * 10;
  store(millibars10, address);
}

// returns pressure as float from address
float retrieve_pressure(int address)
{
  float pressure;
  pressure = retrieve(address) / 10.0;
  return(pressure);
}

// flasher on and off routines
void flasherOn(int Pin)
{
  LEDPin = Pin; 
  Timer1.attachInterrupt(timerIsr);
}

void flasherOff()
{
  Timer1.detachInterrupt();
}

// Custom ISR routine for flashing
void timerIsr()
{
  digitalWrite(LEDPin, digitalRead(LEDPin) ^ 1 );
}

