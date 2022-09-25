#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define MAIN_SCREEN_OLED_RESET -1
#define MAIN_SCREEN_WIDTH 128 // OLED display width, in pixels
#define MAIN_SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MAIN_SCREEN_ADDRESS 0x3C

#define OPTION0_SELECTOR_BOX_HEIGHT 15
#define OPTION0_SELECTOR_BOX_WIDTH 110
#define OPTION1_SELECTOR_BOX_HEIGHT 15
#define OPTION1_SELECTOR_BOX_WIDTH 23

#define OPTIONS_SELECTOR_MAX 3
#define OPTIONSCONFIG_SELECTOR_MAX 11
#define OPTIONSCONFIG_RELAY_SELECTOR_MAX 13

class MainDisplayHandler {
  public:
    MainDisplayHandler();
    void init(TwoWire *twi = &Wire);
    void reset();
    void renderPage(int);
    void setSlavesPage(int);
    void renderOptionsMenu();
    volatile bool IN_OPTIONS_MENU = false;
    volatile uint16_t IN_OPTIONS_PAGE = 0;
    volatile uint16_t IN_OPTIONS_SELECTOR = 0;
    volatile bool IN_OPTIONS_SELECTOR_LOCKED = false;

  private:
    void renderOptions();
    void renderOptionsChannelConfig();
    void renderOptionsOutputMapping();
    unsigned int slaveActivePage;
    // If no value is defined, C++ wil try to assign Adafruit_SH1106G(), which this constructor doesn't exit
    Adafruit_SH1106G _maindisplay = Adafruit_SH1106G(0, 0, &Wire, -1); 
    
};
