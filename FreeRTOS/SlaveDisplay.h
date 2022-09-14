#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SLAVE_SCREEN_OLED_RESET -1
#define SLAVE_SCREEN_WIDTH 128 // OLED display width, in pixels
#define SLAVE_SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SLAVE_SCREEN_ADDRESS 0x3C

static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b11000000,
  0b00000001, 0b11000000,
  0b00000001, 0b11000000,
  0b00000011, 0b11100000,
  0b11110011, 0b11100000,
  0b11111110, 0b11111000,
  0b01111110, 0b11111111,
  0b00110011, 0b10011111,
  0b00011111, 0b11111100,
  0b00001101, 0b01110000,
  0b00011011, 0b10100000,
  0b00111111, 0b11100000,
  0b00111111, 0b11110000,
  0b01111100, 0b11110000,
  0b01110000, 0b01110000,
  0b00000000, 0b00110000 
};

class SlaveDisplay {
  public:
    SlaveDisplay();
    void init(int idx, TwoWire *twi = &Wire);
    void reset();
    void renderTemps(float temps[3]);
    void renderRelayStatus(bool relayState[8]);
    void setSlavesPage(int); 
  private:
    void switchI2CMuxPort(uint8_t port);
    float min, max, var, avg, r_sigma;
    unsigned int slaveActivePage, _idx;
    // If no value is defined, C++ wil try to assign Adafruit_SH1106G(), which this constructor doesn't exit
    Adafruit_SSD1306 _display = Adafruit_SSD1306(0, 0, &Wire, -1);
    TwoWire _twi = TwoWire(0); 

};
