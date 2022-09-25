#include "EEPROMController.h"


EEPROMController::EEPROMController(){
  EEPROMController::reset();
}

bool EEPROMController::init(TwoWire *twi){
    _2Wire = *twi;
    if (ackPolling()) {                                     // See whether the EEPROM responds. We're just starting up so it should be read
        EEPROMStatus = EEPROM_FOUND;
    }
    else {
        EEPROMStatus = EEPROM_NOT_FOUND;
    }
    EEPROMController::reset();
    return EEPROMStatus == EEPROM_FOUND;
}

void EEPROMController::reset(){

}

void EEPROMController::writeEEPROM(uint16_t eeaddress, byte data ) {
    _2Wire.beginTransmission(EEPROM_ADDRESS);
    _2Wire.write(eeaddress >> 8); //writes the MSB
    _2Wire.write(eeaddress & 0xFF); //writes the LSB
    _2Wire.write(data);
    _2Wire.endTransmission();
}

byte EEPROMController::readEEPROM(uint16_t  eeaddress ) {
    byte rdata = 0xFF;
    _2Wire.beginTransmission(EEPROM_ADDRESS);
    _2Wire.write(eeaddress >> 8); //writes the MSB
    _2Wire.write(eeaddress & 0xFF); //writes the LSB
    _2Wire.endTransmission();
    _2Wire.requestFrom(EEPROM_ADDRESS,1);
    if (_2Wire.available())
    {
        rdata = _2Wire.read();
    }
    return rdata;
}

bool EEPROMController::ackPolling() {                                       // Poll the IC to make sure it's ready for communication.
    uint32_t startPolling = micros();
    uint8_t code = 1;
    while (code != 0                                        // Continue Until We Have A Successful Ack, Or
            && micros() - startPolling < 6000) {            // Timeout: Writing Should Not Take More Than 5 Ms, Normal Is ~4.5 Ms.
        _2Wire.beginTransmission(EEPROM_ADDRESS);
        _2Wire.write((uint8_t) 0);
        code = _2Wire.endTransmission();
    }
    return (code == 0);
}