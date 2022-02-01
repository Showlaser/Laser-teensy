#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

class Settings
{
  public:
    String readStringFromEEPROM(int addressOffset);
    void writeStringToEEPROM(int addrOffset, const String &strToWrite);

  private:
};

#endif
