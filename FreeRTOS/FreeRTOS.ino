#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

//#include <U8g2lib.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "AvgStd.h"
// Control Built-in LED
#include <Adafruit_NeoPixel.h>
// Screens
#include <Adafruit_GFX.h>
//#include <Adafruit_SH110X.h>
//#include <Adafruit_SSD1306.h>
#include "MainDisplayHandler.h"
#include "SlaveDisplay.h"

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

//------------<<<< PINs >>>>----------------
// Rotary encoder + button
#define ENCODER_CLK 3
#define ENCODER_DT 2
#define ENCODER_BTN 1
// ESP32 S2 doesn't have hard mapped I2C pins, can be almost any GPIO
#define I2C_1_SDA 9 //6
#define I2C_1_SCL 8 // 7
#define I2C_2_SDA 6 // 11
#define I2C_2_SCL 7 // 12
// Relays
#define RELAY_A_PIN1 42
#define RELAY_A_PIN2 41
#define RELAY_A_PIN3 40
#define RELAY_A_PIN4 39
#define RELAY_B_PIN1 38
#define RELAY_B_PIN2 37
#define RELAY_B_PIN3 36
#define RELAY_B_PIN4 35
// Temp Sensors OneWire
#define ONE_WIRE_BUS_A 33
#define ONE_WIRE_BUS_B 33

//------------<<<< Variables >>>>----------------
uint8_t RelayPins[8] = {RELAY_A_PIN1,RELAY_A_PIN2,RELAY_A_PIN3,RELAY_A_PIN4,
                        RELAY_B_PIN1,RELAY_B_PIN2,RELAY_B_PIN3,RELAY_B_PIN4};

volatile int Counter = 0;
volatile boolean Direction;
volatile boolean ButtonPressed = false;

// ------- Display --------
#define OLED_RESET -1
#define MAIN_SCREEN_WIDTH 128 // OLED display width, in pixels
#define MAIN_SCREEN_HEIGHT 64 // OLED display height, in pixels
#define MAIN_SCREEN_ADDRESS 0x3C
#define SLAVE_SCREEN_WIDTH 128 // OLED display width, in pixels
#define SLAVE_SCREEN_HEIGHT 32 // OLED display height, in pixels
#define SLAVE_SCREEN_ADDRESS 0x3C


TwoWire I2C_PRIMARY = TwoWire(0);
TwoWire I2C_SECONDARY = TwoWire(1);
//Adafruit_SH1106G maindisplayScreen(MAIN_SCREEN_WIDTH, MAIN_SCREEN_HEIGHT, &I2C_PRIMARY, OLED_RESET);
MainDisplayHandler mainDisplay = MainDisplayHandler();
SlaveDisplay slaveDisplay[3] = {SlaveDisplay(),SlaveDisplay(),SlaveDisplay()};
//Adafruit_SSD1306 slaveDisplay(SLAVE_SCREEN_WIDTH, SLAVE_SCREEN_HEIGHT, &I2C_SECONDARY, OLED_RESET);

// ------ NAVIGATION CONTROL -----
static volatile uint8_t CURRENT_PAGE = 0; //uint8_t
#define MENU_PAGE_MAX 4

// Page 0 shows sensors 1 to 6. Page 1 shows 7 to 12;
static volatile uint8_t SLAVE_PAGE = 0; 

// ---- DS18B20 Temp Sensors -------o

// Setup a oneWire instance to communicate with any OneWire device
OneWire oneWireBusA(ONE_WIRE_BUS_A);
// Pass oneWire reference to DallasTemperature library
DallasTemperature sensors(&oneWireBusA);
#define SENSORS_MAX 12
#define SENSOR_ERROR -128

const PROGMEM uint8_t SensorWhite[8] = {0x28,0xF6,0x15,0x60,0x38,0x19,0x01,0x1B};
const PROGMEM uint8_t SensorGrey[8]  = {0x28,0xFE,0xC3,0x2F,0x38,0x19,0x01,0x02};
const PROGMEM uint8_t SensorBlack[8] = {0x28,0xCD,0x25,0x67,0x38,0x19,0x01,0x0C};

// SENSORS ADDRESSES
uint8_t SENSOR_ADDRESS_KNOWN[4][8] = {
  {0x28,0xA0,0xF4,0x79,0xA2,0x00,0x03,0x9B}, //(1) 28A0F479A200039B 
  {0x28,0xA6,0xCF,0x75,0xD0,0x01,0x3C,0x54}, //(2) 28A6CF75D0013C54
  {0x28,0x5E,0xE1,0x75,0xD0,0x01,0x3C,0xDD}, //(3) 285EE175D0013CDD
  {0x28,0xB8,0xC6,0x76,0xE0,0x01,0x3C,0xB9}  //(4) 28B8C676E0013CB9
};
//const PROGMEM uint8_t Sensor3[8] = {0x00,0x0E,0x4C,0x5F,0x52,0x9D,0xC7,0x27}; //4024621965625127 //WHITE, somethign wrong

//const uint8_t* DS18B20[3] = {SensorWhite,SensorGrey,SensorBlack};
uint8_t* SENSOR_ADDRESS_ACTIVE[12] = {};

uint8_t deviceCount = 0;
// Allocating max number of sensors, even if not used.
AvgStd sensorStat[12] = {AvgStd(),AvgStd(),AvgStd(),AvgStd(),
                         AvgStd(),AvgStd(),AvgStd(),AvgStd(),
                         AvgStd(),AvgStd(),AvgStd(),AvgStd()};
float sensorLastReading[12] = {-127,-127,-127,-127,-127,-127,
                               -127,-127,-127,-127,-127,-127};
bool relayState[8] = {false,false,false,false,false,false,false,false};

//---- METHODS / TAKS --------
void IRAM_ATTR ISR_RotaryEncoder();
void IRAM_ATTR ISR_RotaryBTN();

// define two tasks for Blink & AnalogRead
void TaskBlink( void *pvParameters );
void TaskUpdateMainDisplay( void *pvParameters );
void TaskUpdateSlaveDisplays( void *pvParameters );
void TaskReadTempSensors( void *pvParameters );
void TaskUpdateRelays( void *pvParameters );

// the setup function runs once when you press reset or power the board
void setup() {
  
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  Serial.println("Setup starting...");

  initRelayOutputs(); //Doing them first to avoid having them floating
  initEncoderInput();
  initDisplays();
  
  pinMode(ONE_WIRE_BUS_A, INPUT_PULLUP);
  sensors.begin(); // Start up the sensors library
  delay(1000);
  sensors.setResolution(12); // Set resolution to 12 bit
  sensors.requestTemperatures();
  delay(200);
  deviceCount = sensors.getDeviceCount();
  float tempC = sensors.getTempCByIndex(0);
  String nrSensors = "TempSensors Count: " + String(deviceCount);
  Serial.println(nrSensors);
  Serial.println("Sensor 0 Temp: " + String(tempC));

  // Address stuff 
  uint8_t address[8];
  if(sensors.getAddress(address, 0)){
    String addressStr = "";
    int a = 0;
    for(;a<8;++a){
      addressStr += String(address[a]);
    }
    Serial.println("Sensor " + String(0) + " Address:" + addressStr);
  }else{
    Serial.println("Failed to get Address for Sensor " + String(0));
  }

  

  int s = 0;
  
  for(;s < deviceCount; ++s){
    if(sensors.getAddress(address, s)){
      String addressStr = "";
      int a = 0;
      
      Serial.print("Sensor " + String(s) + " Address:");
      printAddress(address);
      Serial.println("");
    }else{
      Serial.println("Failed to get Address for Sensor " + String(s));
    }
  }

  // MAP Sensors to Channels
  for(uint8_t i = 0; i < sizeof(SENSOR_ADDRESS_KNOWN) / sizeof(int); ++i){
    if(sensors.isConnected(SENSOR_ADDRESS_KNOWN[i])){
      SENSOR_ADDRESS_ACTIVE[i] = SENSOR_ADDRESS_KNOWN[i];
    }
  }
  
  // Address stuff END 
  
  /*
  // write number of sensors on first display
  mainDisplay.setFont(u8g2_font_lucasfont_alternate_tr ); 
  mainDisplay.clearBuffer();         // clear the internal memory
  mainDisplay.drawStr(0,16,nrSensors.c_str());  // write something to the internal memory
  mainDisplay.sendBuffer();         // transfer internal memory to the display
*/

  Serial.println("Setup finished...");
  
  // ---- Tasks ----
    // A name just for humans
    // This stack size can be checked & adjusted by reading the Stack Highwater
    // Priority, with 3 (configMAX_PRIORITIES - 1) being the highest, and 0 being the lowest.

  attachInterrupt(ENCODER_CLK, ISR_RotaryEncoder, CHANGE);
  attachInterrupt(ENCODER_DT, ISR_RotaryEncoder, CHANGE);
  attachInterrupt(ENCODER_BTN, ISR_RotaryBTN, RISING);
  
  xTaskCreatePinnedToCore(TaskBlink, "TaskBlink", 1024, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  //xTaskCreatePinnedToCore(TaskAnalogReadA3, "AnalogReadA3", 1024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskUpdateMainDisplay, "UpdateMainDisplay", 4096, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskUpdateSlaveDisplays, "UpdateSlaveDisplays", 4096, NULL, 2, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskReadTempSensors, "ReadTempSensors", 1024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskUpdateRelays, "UpdateRelays", 1024, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

void initDisplays(){
  Serial.println("Init display starting...");
  
  // Setup Main Screen
  I2C_PRIMARY.begin(I2C_1_SDA, I2C_1_SCL, 400000ul);
  mainDisplay.init(&I2C_PRIMARY);

  I2C_SECONDARY.begin(I2C_2_SDA, I2C_2_SCL, 400000ul);
  slaveDisplay[0].init(0, &I2C_SECONDARY);
  slaveDisplay[1].init(1, &I2C_SECONDARY);
  slaveDisplay[2].init(2, &I2C_SECONDARY);

  /*
  slaveDisplay = Adafruit_SSD1306(SLAVE_SCREEN_WIDTH, SLAVE_SCREEN_HEIGHT, &I2C_SECONDARY, OLED_RESET);
  slaveDisplay.setRotation(2);
  switchI2CMuxPort(0);
  if(!slaveDisplay.begin(SSD1306_SWITCHCAPVCC, SLAVE_SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 0 allocation failed"));
  }
  slaveDisplay.display();
  switchI2CMuxPort(1);
  if(!slaveDisplay.begin(SSD1306_SWITCHCAPVCC, SLAVE_SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 1 allocation failed"));
  }
  slaveDisplay.display();
  switchI2CMuxPort(2);
  if(!slaveDisplay.begin(SSD1306_SWITCHCAPVCC, SLAVE_SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 2 allocation failed"));
  }
  slaveDisplay.display();
  
  delay(1000); //Allow Screens to init
  
  //maindisplay.clearDisplay();
  //maindisplay.drawPixel(10, 10, SH110X_WHITE);
  //maindisplay.display();
  switchI2CMuxPort(0);
  slaveDisplay.clearDisplay();
  slaveDisplay.drawPixel(10, 10, SSD1306_WHITE);
  slaveDisplay.display();
  switchI2CMuxPort(1);
  slaveDisplay.clearDisplay();
  slaveDisplay.drawPixel(10, 10, SSD1306_WHITE);
  slaveDisplay.display();
  switchI2CMuxPort(2);
  slaveDisplay.clearDisplay();
  slaveDisplay.drawPixel(10, 10, SSD1306_WHITE);
  slaveDisplay.display();
  delay(1000);
  */
  
  /*
  maindisplay.clearDisplay();
  maindisplay.setTextSize(1);
  maindisplay.setTextColor(SH110X_WHITE);
  maindisplay.setCursor(0,0);
  maindisplay.println("Hello, world! Main");
  maindisplay.display();
  int i = 0;
  for(;i<3;++i){
    switchI2CMuxPort(i); // SLAVE diplay 1 (idx 0)
    slaveDisplay.clearDisplay();
    slaveDisplay.setTextSize(1);
    slaveDisplay.setTextColor(SSD1306_WHITE);
    slaveDisplay.setCursor(0,0);
    slaveDisplay.print("Hello, world!");
    slaveDisplay.println(i);
    slaveDisplay.display();
  }

  
  delay(2000);
  i = 0;
  for(;i<3;++i){
    switchI2CMuxPort(i); // SLAVE diplay 1 (idx 0)
    slaveDisplay.clearDisplay();
    slaveDisplay.drawLine(0, 0, SLAVE_SCREEN_WIDTH-1, 0, SH110X_WHITE); // TEMP Horizontal Line
    slaveDisplay.drawLine(0, 0, 0, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(42, 0, 42, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(84, 0, 84, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(SLAVE_SCREEN_WIDTH-1, 0, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(0, SLAVE_SCREEN_HEIGHT - 1, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE); // TEMP Horizontal Line
    slaveDisplay.display();
  }*/
  
  Serial.println("Init display finshed!");
}


void initEncoderInput(){
  Serial.println("Init Encoder...");
  pinMode(ENCODER_CLK, INPUT); //INPUT_PULLUP
  pinMode(ENCODER_DT, INPUT);
  pinMode(ENCODER_BTN, INPUT_PULLUP);
    
  // ... and their pull-up resistors activated
  //digitalWrite(ENCODER_CLK, true);
  //digitalWrite(ENCODER_DT, true);
  //digitalWrite(ENCODER_BTN, true);
    
  Serial.println("Init Encoder finished!");
}

void initRelayOutputs(){
  Serial.println("Init initRelayOutputs...");
  pinMode(RELAY_A_PIN1, OUTPUT); 
  pinMode(RELAY_A_PIN2, OUTPUT);
  pinMode(RELAY_A_PIN3, OUTPUT);
  pinMode(RELAY_A_PIN4, OUTPUT); 
  pinMode(RELAY_B_PIN1, OUTPUT);
  pinMode(RELAY_B_PIN2, OUTPUT);
  pinMode(RELAY_B_PIN3, OUTPUT); 
  pinMode(RELAY_B_PIN4, OUTPUT);

  // Set all relays to Off
  digitalWrite(RELAY_A_PIN1, true); 
  digitalWrite(RELAY_A_PIN2, true);
  digitalWrite(RELAY_A_PIN3, true);
  digitalWrite(RELAY_A_PIN4, true);
  digitalWrite(RELAY_B_PIN1, true); 
  digitalWrite(RELAY_B_PIN2, true);
  digitalWrite(RELAY_B_PIN3, true);
  digitalWrite(RELAY_B_PIN4, true);
  Serial.println("Init initRelayOutputs finished!");
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/

void TaskBlink(void *pvParameters)  
{
  (void) pvParameters;

  // initialize digital LED_BUILTIN on pin 13 as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  for (;;) // A Task shall never return or exit.
  {
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    vTaskDelay(900);  // one tick delay (15ms) in between reads for stability
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    vTaskDelay(900);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskReadTempSensors(void *pvParameters)
{
  (void) pvParameters;
  
  for (;;)
  {
    //Serial.println("MainDisplayTask called \n");

    // Send command to all the sensors for temperature conversion
    sensors.requestTemperatures();
    // Display temperature from each sensor
    for (int i = 0;  i < SENSORS_MAX ;  i++)
    {
      if(SENSOR_ADDRESS_ACTIVE[i]){
        float tempC = sensors.getTempC(SENSOR_ADDRESS_ACTIVE[i]);
        //float tempC = sensors.getTempCByIndex(i);
        if(tempC == DEVICE_DISCONNECTED_C){
          sensorLastReading[i] = SENSOR_ERROR; // Using -128 to 
        }else{
          sensorLastReading[i] = tempC;
          sensorStat[i].checkAndAddReading(tempC);
        } 
      }
      
    }
    vTaskDelay(1000);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskUpdateMainDisplay(void *pvParameters) 
{
  (void) pvParameters;
  
  for (;;)
  { 
    mainDisplay.setSlavesPage(SLAVE_PAGE);
    mainDisplay.renderPage(CURRENT_PAGE);
    
    if(ButtonPressed){
      Serial.println("BUTTON PRESS");
      vTaskDelay(100);
      ButtonPressed = false;
    }
    vTaskDelay(100);  // one tick delay (15ms) in between reads for stability   
  }
}

void TaskUpdateSlaveDisplays(void *pvParameters)
{
  (void) pvParameters;
  float temps[3];
  
  for (;;)
  {
    if(!SLAVE_PAGE){
      temps[0] = sensorLastReading[0];
      temps[1] = getSensorSlaveValue(1);
      temps[2] = getSensorSlaveValue(2);
      slaveDisplay[0].renderTemps(temps);
      temps[0] = getSensorSlaveValue(3);
      temps[1] = getSensorSlaveValue(4);
      temps[2] = getSensorSlaveValue(5);
      slaveDisplay[1].renderTemps(temps);
    }
    else
    {
      temps[0] = getSensorSlaveValue(6);
      temps[1] = getSensorSlaveValue(7);
      temps[2] = getSensorSlaveValue(8);
      slaveDisplay[0].renderTemps(temps);
      temps[0] = getSensorSlaveValue(9);
      temps[1] = getSensorSlaveValue(10);
      temps[2] = getSensorSlaveValue(11);
      slaveDisplay[1].renderTemps(temps);
    }    
    
    slaveDisplay[2].renderRelayStatus(relayState);
    
    /*
    for(i = (SLAVE_PAGE*6); i < ((SLAVE_PAGE+1)*6);){
      
      if(i<3 || (i>=6 && i<9)){
        switchI2CMuxPort(0); // SLAVE diplay 1 (idx 0)
      }else if((i>=3 && i<6) || (i>=9 && i<12)){
        switchI2CMuxPort(1); // SLAVE diplay 1 (idx 0)
      }

      slaveDisplay.clearDisplay();
      slaveDisplay.setTextSize(1);
      slaveDisplay.setTextColor(SSD1306_WHITE);
      for(j = 0; j < 3; ++j, ++i){
        if(sensorLastReading[i] != DEVICE_DISCONNECTED_C){
          slaveDisplay.setCursor(j*42+7,12);
          slaveDisplay.print(getSensorSlaveValue(i));
        }else if(sensorLastReading[i] == SENSOR_ERROR){
          slaveDisplay.setCursor(j*42+12,12);
          slaveDisplay.print("ERR");
        }else{
          slaveDisplay.setCursor(j*42+17,12);
          slaveDisplay.print("NA");
        }
      }
      slaveDisplay.drawLine(42, 0, 42, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
      slaveDisplay.drawLine(84, 0, 84, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
      slaveDisplay.display();
    }
*/

/*
    // ------ UDPATE Relays Status Display -------------
    switchI2CMuxPort(2);

    slaveDisplay.clearDisplay();
    // Horizontal Lines
    slaveDisplay.drawLine(0, 0, SLAVE_SCREEN_WIDTH-1, 0, SH110X_WHITE);
    slaveDisplay.drawLine(0, 16, SLAVE_SCREEN_WIDTH-1, 16, SH110X_WHITE);
    slaveDisplay.drawLine(0, SLAVE_SCREEN_HEIGHT - 1, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    // Vertical Lines
    slaveDisplay.drawLine(0, 0, 0, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(32, 0, 32, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(64, 0, 64, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(96, 0, 96, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);
    slaveDisplay.drawLine(SLAVE_SCREEN_WIDTH-1, 0, SLAVE_SCREEN_WIDTH-1, SLAVE_SCREEN_HEIGHT - 1, SH110X_WHITE);

    uint8_t recX = 2;
    uint8_t recY = 2;
    for(i=0;i<8;++i){
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
        slaveDisplay.writeFillRect(recX,recY,27,11,SH110X_WHITE);
        slaveDisplay.setTextColor(SH110X_BLACK);
        slaveDisplay.setCursor(recX+12,recY+2);
        slaveDisplay.print(i+1);
      }else{
        slaveDisplay.writeFillRect(recX,recY,27,11,SH110X_BLACK);
        slaveDisplay.setTextColor(SH110X_WHITE);
        slaveDisplay.setCursor(recX+12,recY+2);
        slaveDisplay.print(i+1);
      }
    }
    slaveDisplay.display();
    */
    vTaskDelay(400);  // one tick delay (15ms) in between reads for stability   
  }
}

void TaskUpdateRelays(void *pvParameters)  
{
  (void) pvParameters;
  int i = 0;
  
  for (;;)
  {
    for(i = 0; i < 8; ++i){
      if(sensorLastReading[i] != -127){
        /*
        if(!relayState[i]){ //Relay is OFF
          if(sensorLastReading[i] >= 17.5 ||){
            relayState[i] = true;
            digitalWrite(RelayPins[i], false); 
          }
        }else{ // Relay is ON (Cooling)
          if(sensorLastReading[i] <= 16.5){
            relayState[i] = false;
            digitalWrite(RelayPins[i], true); 
          }
        }*/
         
        if(sensorLastReading[i] >= 17.5){  // If reached High Threshold
        // || (!relayState[i] && sensorLastReading[i] > 16.5 && sensorLastReading[i] < 17.5)){ //or we are in target window and not cooling
          relayState[i] = true;
          digitalWrite(RelayPins[i], false); 
        }
        else if(sensorLastReading[i] <= 16.5){
          relayState[i] = false;
          digitalWrite(RelayPins[i], true); 
        }
      }
      
    }
    vTaskDelay(950);  // one tick delay (15ms) in between reads for stability   
  }
}

// --------- INTERRUPT ROUTINES -----
void IRAM_ATTR ISR_RotaryEncoder(void) 
{
    static uint8_t old_AB = 3;  // Lookup table index
    static int8_t encval = 0;   // Encoder value  
    static const int8_t enc_states[]  = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0}; // Lookup table
  
    old_AB <<=2;  // Remember previous state
  
    if (digitalRead(ENCODER_CLK)) old_AB |= 0x02; // Add current state of pin A
    if (digitalRead(ENCODER_DT)) old_AB |= 0x01; // Add current state of pin B
    
    encval += enc_states[( old_AB & 0x0f )];
  
    // Update counter if encoder has rotated a full indent, that is at least 4 steps
    if( encval > 3) {        // Four steps forward
      if(CURRENT_PAGE < MENU_PAGE_MAX){
        CURRENT_PAGE++;  
      }
      encval = 0;
    }
    else if( encval < -3 ) {  // Four steps backwards
     if(CURRENT_PAGE!=0){
      CURRENT_PAGE--;// Decrease counter
     }
     encval = 0;
    }
}

void IRAM_ATTR ISR_RotaryBTN(void)
{
    noInterrupts();
    ButtonPressed = true;
    if(SLAVE_PAGE == 0){ SLAVE_PAGE = 1; }
    else{ SLAVE_PAGE = 0; }
    interrupts();
}

// --------- HELPER METHODS ---------
void switchI2CMuxPort(uint8_t port)
{
  I2C_SECONDARY.beginTransmission(0x70);
  I2C_SECONDARY.write(1 << port);
  I2C_SECONDARY.endTransmission();
  //Serial.print("Switched to I2C Mux Port ");
  //Serial.println(port);
}

// Returns the float value depending on active Page
float getSensorSlaveValue(uint8_t sensorIdx){
  
  switch(CURRENT_PAGE){
      case 0:
        return sensorLastReading[sensorIdx];
      case 1: 
        return sensorStat[sensorIdx].getMean();
      case 2:
        return sensorStat[sensorIdx].getMax();
      case 3:
        return sensorStat[sensorIdx].getMin();
      case 4:
        return sensorStat[sensorIdx].getVariance();
    }
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}
