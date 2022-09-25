#include "SlaveDisplay.h"


SlaveDisplay::SlaveDisplay(){
  SlaveDisplay::reset();
}

void SlaveDisplay::init(int idx, TwoWire *twi){
    _idx = idx;
    _display = Adafruit_SSD1306(SLAVE_SCREEN_WIDTH, SLAVE_SCREEN_HEIGHT, twi, SLAVE_SCREEN_OLED_RESET);
    _twi = *twi;
    _display.setRotation(2);
    SlaveDisplay::switchI2CMuxPort(idx);
    if(!_display.begin(SSD1306_SWITCHCAPVCC, SLAVE_SCREEN_ADDRESS)) {
      Serial.println(F("SSD1306 0 allocation failed"));
    }
    _display.display();
    SlaveDisplay::reset();
}

void SlaveDisplay::reset(){
    slaveActivePage = 0;
    avg = 0;
    var = 0;
    min = 0;
    max = 0;
    r_sigma = -1;
}

void SlaveDisplay::setSlavesPage(int slavePageIndex){
  slaveActivePage = slavePageIndex;
}

void SlaveDisplay::renderTemps(float temps[3]){
    SlaveDisplay::switchI2CMuxPort(_idx);
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(WHITE);

    for(int i = 0; i < 3; ++i){
      if(temps[i] > -127){
        _display.setCursor(i*42+7,12);
        _display.print(temps[i]);
      }else if(temps[i] == -128){
        _display.setCursor(i*42+12,12);
        _display.print("ERR");
      }else{
        _display.setCursor(i*42+17,12);
        _display.print("NA");
      }
    }
    
    _display.drawLine(42, 0, 42, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.drawLine(84, 0, 84, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.display();
}

void SlaveDisplay::renderRelayStatus(bool relayState[8]){
    SlaveDisplay::switchI2CMuxPort(_idx);
    _display.clearDisplay();
    
    // Horizontal Lines
    _display.drawLine(0, 0, SLAVE_SCREEN_WIDTH-1, 0, WHITE);
    _display.drawLine(0, 16, SLAVE_SCREEN_WIDTH-1, 16, WHITE);
    _display.drawLine(0, SLAVE_SCREEN_HEIGHT - 1, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    // Vertical Lines
    _display.drawLine(0, 0, 0, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.drawLine(32, 0, 32, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.drawLine(64, 0, 64, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.drawLine(96, 0, 96, SLAVE_SCREEN_HEIGHT - 1, WHITE);
    _display.drawLine(SLAVE_SCREEN_WIDTH-1, 0, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, WHITE);

    uint8_t recX = 2;
    uint8_t recY = 2;
    for(int i=0; i<8; ++i){
      if(i<4){
        recX = (32*i)+3;
        recY = 3;
      }
      else
      {
        recX = 32*(i-4)+3;
        recY = 19;
      }
      
      if(relayState[i]){
        // x, y, w, h
        _display.writeFillRect(recX,recY,27,11,WHITE);
        _display.setTextColor(BLACK);
        _display.setCursor(recX+12,recY+2);
        _display.print(i+1);
      }else{
        _display.writeFillRect(recX,recY,27,11,BLACK);
        _display.setTextColor(WHITE);
        _display.setCursor(recX+12,recY+2);
        _display.print(i+1);
      }
    }
    _display.display();
}

void SlaveDisplay::renderText(String txt, bool selected, bool textSizeLarge){
    SlaveDisplay::switchI2CMuxPort(_idx);
    _display.clearDisplay();
    _display.setTextColor(WHITE);

    if(textSizeLarge)
    {
      _display.setTextSize(2);
      _display.setCursor(0,0);
      _display.print(txt);
    }
    else
    {
      _display.setTextSize(1);
      _display.setCursor(0,15);
      _display.print(txt);
    }
    
    
    if(selected)
    {
      // Horizontal
      _display.drawLine(0, 0, SLAVE_SCREEN_WIDTH-1, 0, WHITE);
      _display.drawLine(0, SLAVE_SCREEN_HEIGHT-1, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT-1, WHITE);
      // Vertical
      _display.drawLine(0, 0, 0, SLAVE_SCREEN_HEIGHT-1, WHITE);
      _display.drawLine(SLAVE_SCREEN_WIDTH-1, 0, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT-1, WHITE);
    }

    _display.display();
}

void SlaveDisplay::renderOption(String txt, float val, bool selected, bool textSizeLarge){
    SlaveDisplay::switchI2CMuxPort(_idx);
    _display.clearDisplay();
    _display.setTextColor(WHITE);
    if(textSizeLarge)
    {
      _display.setTextSize(2);
      _display.setCursor(0,0);
      _display.print(txt);
      _display.setCursor(74,15);
      _display.print(val,1);
    }
    else
    {
      _display.setTextSize(1);
      _display.setCursor(5,15);
      _display.print(txt);
      _display.setCursor(74,15);
      _display.print(val,1);
    }

    if(selected)
    {
      // Horizontal
      _display.drawLine(68, 10, 100, 10, WHITE);
      _display.drawLine(68, 30, 100, 30, WHITE);
      // Vertical
      _display.drawLine(68, 10, 68, 30, WHITE);
      _display.drawLine(100, 10, 100, 30, WHITE);
    }

    _display.display();
}

/*
void SlaveDisplay::renderPage(int pageIndex){
    switchI2CMuxPort(idx);
    _display.clearDisplay();
    _display.setTextSize(1);
    _display.setTextColor(SSD1306_WHITE);
    
    for(i = (SLAVE_PAGE*6); i < ((SLAVE_PAGE+1)*6);){
      
      if(i<3 || (i>=6 && i<9)){
        switchI2CMuxPort(0); // SLAVE diplay 1 (idx 0)
      }else if((i>=3 && i<6) || (i>=9 && i<12)){
        switchI2CMuxPort(1); // SLAVE diplay 1 (idx 0)
      }


      
      for(j = 0; j < 3; ++j, ++i){
        if(sensorLastReading[i] != DEVICE_DISCONNECTED_C){
          _display.setCursor(j*42+7,12);
          _display.print(getSensorSlaveValue(i));
        }else if(sensorLastReading[i] == SENSOR_ERROR){
          _display.setCursor(j*42+12,12);
          _display.print("ERR");
        }else{
          _display.setCursor(j*42+17,12);
          _display.print("NA");
        }
      }
      _display.drawLine(42, 0, 42, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
      _display.drawLine(84, 0, 84, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
      _display.display();
    }

}
*/

// --------- HELPER METHODS ---------
void SlaveDisplay::switchI2CMuxPort(uint8_t port)
{
  _twi.beginTransmission(0x70);
  _twi.write(1 << port);
  _twi.endTransmission();
}
