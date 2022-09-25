#include <Wire.h>
#include "Arduino.h"

#define EEPROM_ADDRESS 0x50

class EEPROMController{
  public:
    EEPROMController();
    bool init(TwoWire *twi = &Wire);
    void reset();
    enum Status {
      UNKNOWN,
      EEPROM_NOT_FOUND,
      EEPROM_FOUND
    } EEPROMStatus = UNKNOWN;

  private:
    TwoWire _2Wire = TwoWire(0);
    void writeEEPROM(uint16_t eeaddress, byte data);
    byte readEEPROM(uint16_t eeaddress);
    bool ackPolling();
    unsigned int memAddress;
};
