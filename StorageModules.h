#ifndef StorageModulesH
#define StorageModulesH

void siren();
void countout(int c);
void store(int z, int address);
int retrieve(int address);
void store_time(long duration, int address);
float retrieve_time(int address);
void store_pressure(float pressure, int address);
float retrieve_pressure(int address);
void flasherOn(int Pin);
void flasherOff();

#endif
