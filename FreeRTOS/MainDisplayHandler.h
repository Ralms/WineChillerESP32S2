#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define MAIN_SCREEN_OLED_RESET -1
#define MAIN_SCREEN_WIDTH 128 // OLED display width, in pixels
#define MAIN_SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MAIN_SCREEN_ADDRESS 0x3C

class MainDisplayHandler {
  public:
    MainDisplayHandler();
    void init(TwoWire *twi = &Wire);
    void reset();
    void renderPage(int);
    void setSlavesPage(int);
  private:
    float min, max, var, avg, r_sigma;
    unsigned int slaveActivePage;
    // If no value is defined, C++ wil try to assign Adafruit_SH1106G(), which this constructor doesn't exit
    Adafruit_SH1106G _maindisplay = Adafruit_SH1106G(0, 0, &Wire, -1); 
    
};
