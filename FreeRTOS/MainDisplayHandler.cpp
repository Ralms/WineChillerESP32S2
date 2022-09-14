#include "MainDisplayHandler.h"


#define LOGO16_GLCD_HEIGHT 16
#define LOGO16_GLCD_WIDTH  16
static const unsigned char PROGMEM logo16_glcd_bmp[] =
{ B00000000, B11000000,
  B00000001, B11000000,
  B00000001, B11000000,
  B00000011, B11100000,
  B11110011, B11100000,
  B11111110, B11111000,
  B01111110, B11111111,
  B00110011, B10011111,
  B00011111, B11111100,
  B00001101, B01110000,
  B00011011, B10100000,
  B00111111, B11100000,
  B00111111, B11110000,
  B01111100, B11110000,
  B01110000, B01110000,
  B00000000, B00110000
};

MainDisplayHandler::MainDisplayHandler(){
  MainDisplayHandler::reset();
}

void MainDisplayHandler::init(TwoWire *twi){
    _maindisplay = Adafruit_SH1106G(MAIN_SCREEN_WIDTH, MAIN_SCREEN_HEIGHT, twi, MAIN_SCREEN_OLED_RESET);
    _maindisplay.begin(MAIN_SCREEN_ADDRESS, true);
    _maindisplay.display();
    MainDisplayHandler::reset();
}

void MainDisplayHandler::reset(){
    slaveActivePage = 0;
    avg = 0;
    var = 0;
    min = 0;
    max = 0;
    r_sigma = -1;
}

void MainDisplayHandler::setSlavesPage(int slavePageIndex){
  slaveActivePage = slavePageIndex;
}

void MainDisplayHandler::renderPage(int pageIndex){
    _maindisplay.clearDisplay();
    _maindisplay.setTextSize(1);
    // ------ GRID SLAVES STATUS ---------
    // Horizontal Lines
    _maindisplay.drawLine(0, 0, 42, 0, SH110X_WHITE);
    _maindisplay.drawLine(0, 21, 42, 21, SH110X_WHITE);
    _maindisplay.drawLine(0, 42, 42, 42, SH110X_WHITE);
    _maindisplay.drawLine(0, MAIN_SCREEN_HEIGHT - 1, 42, MAIN_SCREEN_HEIGHT - 1, SH110X_WHITE);
    // Vertical Lines
    _maindisplay.drawLine(42, 0, 42, MAIN_SCREEN_HEIGHT - 1, SH110X_WHITE);
    
    _maindisplay.setTextColor(SH110X_WHITE);
    if(!slaveActivePage){
      _maindisplay.setCursor(11,8);
      _maindisplay.print("1-3");
      _maindisplay.setCursor(11,29);
      _maindisplay.print("4-6");
    }
    else
    {
      _maindisplay.setCursor(11,8);
      _maindisplay.print("7-9");
      _maindisplay.setCursor(4,29);
      _maindisplay.print("10-12");
    }
    _maindisplay.setCursor(2,50);
    _maindisplay.print("Relays");
    // ------ GRID SLAVES STATUS END ---------

    
    _maindisplay.setTextSize(2);
    _maindisplay.setCursor(52,25);
    switch(pageIndex){
      case 0:
        _maindisplay.print("Status");
        break;
      case 1: 
        _maindisplay.print("Avg");
        break;
      case 2:
        _maindisplay.print("Max");
        break;
      case 3:
        _maindisplay.print("Min");
        break;
      case 4:
        _maindisplay.print("Var");
        break;
    }

    //maindisplay.setTextSize(1);
    //maindisplay.setCursor(54,0);
    //maindisplay.print(CURRENT_PAGE);
    
    _maindisplay.display();
}
