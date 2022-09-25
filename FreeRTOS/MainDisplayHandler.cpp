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

void MainDisplayHandler::renderOptionsMenu(){
  _maindisplay.clearDisplay();
  _maindisplay.setTextSize(1);

  // Set Option Page title
  _maindisplay.setRotation(3);
  _maindisplay.drawLine(0, MAIN_SCREEN_WIDTH-12, MAIN_SCREEN_HEIGHT-1, MAIN_SCREEN_WIDTH-12, SH110X_WHITE);
  _maindisplay.setCursor(0, MAIN_SCREEN_WIDTH-9);
  switch (IN_OPTIONS_PAGE)
  {
    case 0:
      _maindisplay.print("OPTIONS");
      _maindisplay.setRotation(0);
      MainDisplayHandler::renderOptions();
      break;
    case 1:
      _maindisplay.print("CH. CONFIG");
      _maindisplay.setRotation(0);
      MainDisplayHandler::renderOptionsChannelConfig();
      break;
    case 2:
      _maindisplay.print("OUTPUT MAP");
      _maindisplay.setRotation(0);
      MainDisplayHandler::renderOptionsOutputMapping();
      break;
  }

  _maindisplay.display();
}

void MainDisplayHandler::renderOptions(){
  _maindisplay.setCursor(5,4);
  _maindisplay.print("Channel Config");
  _maindisplay.setCursor(5,20);
  _maindisplay.print("Output Mapping");
  _maindisplay.setCursor(5,34);
  _maindisplay.print("Switch Bank");
  _maindisplay.setCursor(5,50);
  _maindisplay.print("<-- Exit");

  //---- Highlighted option:------
  u_int16_t selectorBoxY = IN_OPTIONS_SELECTOR * OPTION0_SELECTOR_BOX_HEIGHT;
  //vertical lines
  _maindisplay.drawLine(0, selectorBoxY, 0, selectorBoxY+OPTION0_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  _maindisplay.drawLine(OPTION0_SELECTOR_BOX_WIDTH, selectorBoxY, OPTION0_SELECTOR_BOX_WIDTH, selectorBoxY+OPTION0_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  //horizontal lines
  _maindisplay.drawLine(0, selectorBoxY, OPTION0_SELECTOR_BOX_WIDTH, selectorBoxY, SH110X_WHITE);
  _maindisplay.drawLine(0, selectorBoxY+OPTION0_SELECTOR_BOX_HEIGHT, OPTION0_SELECTOR_BOX_WIDTH, selectorBoxY+OPTION0_SELECTOR_BOX_HEIGHT, SH110X_WHITE);

  // _maindisplay.display(); called at renderOptionsMenu()
}

void MainDisplayHandler::renderOptionsChannelConfig(){

  _maindisplay.setCursor(30,10);
  _maindisplay.print("Channel ");;
  _maindisplay.print(IN_OPTIONS_SELECTOR);

  _maindisplay.setCursor(5,35);
  _maindisplay.print("Curr. Read: ");
  _maindisplay.print(22.7);

  // --- Bottom Menu ---
  _maindisplay.drawLine(0, 50, MAIN_SCREEN_WIDTH-12, 50, SH110X_WHITE);
  _maindisplay.drawLine(60, 50, 60, MAIN_SCREEN_HEIGHT-1, SH110X_WHITE);
  _maindisplay.setCursor(20,MAIN_SCREEN_HEIGHT-9);
  _maindisplay.print("Save");
  _maindisplay.setCursor(80,MAIN_SCREEN_HEIGHT-9);
  _maindisplay.print("Back");

  /* // Spent time doing the grid, to then realize it doesn't make sense to have it. 
  for(int i = 0; i < 4; ++i){
    _maindisplay.setCursor((i*24)+18,8);
    _maindisplay.print(i+1);  
  }
  for(int i = 0; i < 4; ++i){
    _maindisplay.setCursor((i*24)+18,26);
    _maindisplay.print(i+5);  
  }
  _maindisplay.setCursor(18,45);
  _maindisplay.print(9); // seperating 9, in order to center 10, 11 and 12
  for(int i = 1; i < 4; ++i){
    _maindisplay.setCursor((i*24)+15,45);
    _maindisplay.print(i+9);  
  }

  u_int16_t selectorBoxX = (IN_OPTIONS_SELECTOR * OPTION1_SELECTOR_BOX_WIDTH) + 10;
  u_int16_t selectorBoxY = (IN_OPTIONS_SELECTOR * OPTION1_SELECTOR_BOX_HEIGHT) + 2;
  //vertical lines
  _maindisplay.drawLine(selectorBoxX, selectorBoxY, selectorBoxX, selectorBoxY + OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  _maindisplay.drawLine(selectorBoxX + OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY, selectorBoxX + OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY + OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);

  //horizontal lines
  _maindisplay.drawLine(selectorBoxX, selectorBoxY, selectorBoxX +OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY, SH110X_WHITE);
  _maindisplay.drawLine(selectorBoxX, selectorBoxY+OPTION1_SELECTOR_BOX_HEIGHT, selectorBoxX+OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY+OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  */


  // _maindisplay.display(); called at renderOptionsMenu()
}

void MainDisplayHandler::renderOptionsOutputMapping(){

  for(int i = 0; i < 4; ++i){
    _maindisplay.setCursor((i*24)+18,4);
    _maindisplay.print(i+1);  
  }
  for(int i = 0; i < 4; ++i){
    _maindisplay.setCursor((i*24)+18,19);
    _maindisplay.print(i+5);  
  }
  _maindisplay.setCursor(18,35);
  _maindisplay.print(9); // seperating 9, in order to center 10, 11 and 12
  for(int i = 1; i < 4; ++i){
    _maindisplay.setCursor((i*24)+15,35);
    _maindisplay.print(i+9);  
  }


  // ------- SELECTOR BOX -----------

  u_int16_t selectorBoxX = 0;
  u_int16_t selectorBoxY = 0;

  if(IN_OPTIONS_SELECTOR >= 0 && IN_OPTIONS_SELECTOR < 4)
  {
    selectorBoxX = (IN_OPTIONS_SELECTOR * OPTION1_SELECTOR_BOX_WIDTH) + 10;
    selectorBoxY = 0;
  }
  else if(IN_OPTIONS_SELECTOR >= 4 && IN_OPTIONS_SELECTOR < 8)
  {
    selectorBoxX = ((IN_OPTIONS_SELECTOR-4) * OPTION1_SELECTOR_BOX_WIDTH) + 10;
    selectorBoxY = OPTION1_SELECTOR_BOX_HEIGHT;
  }
  else if(IN_OPTIONS_SELECTOR >= 8 && IN_OPTIONS_SELECTOR < 12)
  {
    selectorBoxX = ((IN_OPTIONS_SELECTOR-8) * OPTION1_SELECTOR_BOX_WIDTH) + 10;
    selectorBoxY = OPTION1_SELECTOR_BOX_HEIGHT *2;
  }
  
  //vertical lines
  _maindisplay.drawLine(selectorBoxX, selectorBoxY, selectorBoxX, selectorBoxY + OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  _maindisplay.drawLine(selectorBoxX + OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY, selectorBoxX + OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY + OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);

  //horizontal lines
  _maindisplay.drawLine(selectorBoxX, selectorBoxY, selectorBoxX +OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY, SH110X_WHITE);
  _maindisplay.drawLine(selectorBoxX, selectorBoxY+OPTION1_SELECTOR_BOX_HEIGHT, selectorBoxX+OPTION1_SELECTOR_BOX_WIDTH, selectorBoxY+OPTION1_SELECTOR_BOX_HEIGHT, SH110X_WHITE);
  
  // --- Bottom Menu ---
  _maindisplay.drawLine(0, 50, MAIN_SCREEN_WIDTH-12, 50, SH110X_WHITE);
  _maindisplay.drawLine(60, 50, 60, MAIN_SCREEN_HEIGHT-1, SH110X_WHITE);
  _maindisplay.setCursor(20,MAIN_SCREEN_HEIGHT-9);
  _maindisplay.print("Save");
  _maindisplay.setCursor(80,MAIN_SCREEN_HEIGHT-9);
  _maindisplay.print("Back");

  // _maindisplay.display(); called at renderOptionsMenu()
}
