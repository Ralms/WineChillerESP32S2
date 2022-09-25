#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

//#include <U8g2lib.h>
#include <stdio.h>
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
#include "EEPROMController.h"

//#include "ChannelConfig.h"
#include "SensorController.h" // Sensor V2

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

//------------<<<< PINs >>>>----------------
// Rotary encoder + button
#define ENCODER_CLK 3
#define ENCODER_DT 2
#define ENCODER_BTN 1
// ESP32 S2 doesn't have hard mapped I2C pins, can be almost any GPIO
#define I2C_1_SDA 8 //6
#define I2C_1_SCL 9 // 7
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
#define ONE_WIRE_BUS_A 34
#define ONE_WIRE_BUS_B 33

//------------<<<< Variables >>>>----------------
uint8_t RelayPins[8] = {RELAY_A_PIN1,RELAY_A_PIN2,RELAY_A_PIN3,RELAY_A_PIN4,
                        RELAY_B_PIN1,RELAY_B_PIN2,RELAY_B_PIN3,RELAY_B_PIN4};

volatile TickType_t lastBTNPressTickCount = 0;
volatile boolean Direction;

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
MainDisplayHandler mainDisplay = MainDisplayHandler();
SlaveDisplay slaveDisplay[3] = {SlaveDisplay(),SlaveDisplay(),SlaveDisplay()};

// ------ NAVIGATION CONTROL -----
static volatile uint8_t CURRENT_PAGE = 0;
#define MENU_PAGE_MAX 4

// Bank 0 shows sensors 1 to 6. Bank 1 shows 7 to 12;
static volatile uint8_t CHANNEL_BANK = 0; 

// ---- DS18B20 Temp Sensors -------o

// Sensor V1
// Setup a oneWire instance to communicate with any OneWire device
//OneWire oneWireBusA(ONE_WIRE_BUS_A); // DELETE
//OneWire oneWireBusA(ONE_WIRE_BUS_A);

// Pass oneWire reference to DallasTemperature library
//DallasTemperature sensors(&oneWireBusA); // DELETE

#define SENSORS_MAX 12
#define SENSOR_ERROR -128

// const PROGMEM uint8_t SensorWhite[8] = {0x28,0xF6,0x15,0x60,0x38,0x19,0x01,0x1B};
// const PROGMEM uint8_t SensorGrey[8]  = {0x28,0xFE,0xC3,0x2F,0x38,0x19,0x01,0x02};
// const PROGMEM uint8_t SensorBlack[8] = {0x28,0xCD,0x25,0x67,0x38,0x19,0x01,0x0C};

// SENSORS ADDRESSES
uint8_t SENSOR_ADDRESS_KNOWN[4][8] = {
  {0x28,0xA0,0xF4,0x79,0xA2,0x00,0x03,0x9B}, //(1) 28A0F479A200039B 
  {0x28,0xA6,0xCF,0x75,0xD0,0x01,0x3C,0x54}, //(2) 28A6CF75D0013C54
  {0x28,0x5E,0xE1,0x75,0xD0,0x01,0x3C,0xDD}, //(3) 285EE175D0013CDD
  {0x28,0xB8,0xC6,0x76,0xE0,0x01,0x3C,0xB9}  //(4) 28B8C676E0013CB9
};
//const PROGMEM uint8_t Sensor3[8] = {0x00,0x0E,0x4C,0x5F,0x52,0x9D,0xC7,0x27}; //4024621965625127 //WHITE, somethign wrong

ChannelConfig channelsConfig[12] = {
  ChannelConfig(0,SENSOR_ADDRESS_KNOWN[0], (float)17, (float)0.5),
  ChannelConfig(1,SENSOR_ADDRESS_KNOWN[1], (float)17, (float)0.5),
  ChannelConfig(2,SENSOR_ADDRESS_KNOWN[2], (float)17, (float)0.5),
  ChannelConfig(3,SENSOR_ADDRESS_KNOWN[3], (float)17, (float)0.5),
  ChannelConfig(4, 0, 0, 0),
  ChannelConfig(5, 0, 0, 0),
  ChannelConfig(6, 0, 0, 0),
  ChannelConfig(7, 0, 0, 0),
  ChannelConfig(8, 0, 0, 0),
  ChannelConfig(9, 0, (float)10, (float)1),
  ChannelConfig(10, 0, (float)10, (float)1),
  ChannelConfig(11, 0, (float)10, (float)1)
};

SensorController _sensorController = SensorController(); // Sensor V2
OneWire oneWireBusA(ONE_WIRE_BUS_A);
OneWire oneWireBusB(ONE_WIRE_BUS_B);
EEPROMController _eepromController = EEPROMController(); // EEPROM settings storage

uint16_t relayMapping[8] = {66,3,1,2,66,66,66,66}; // CHANGE TO DYNAMIC STORAGE

// ----------- GLOBAL RUNTIME Variables ------------

bool _selectedBlink = true;

uint8_t* SENSOR_ADDRESS_ACTIVE[12] = {};

uint8_t deviceCount = 0;
// Allocating max number of sensors, even if not used.
AvgStd sensorStat[12] = {AvgStd(),AvgStd(),AvgStd(),AvgStd(),
                         AvgStd(),AvgStd(),AvgStd(),AvgStd(),
                         AvgStd(),AvgStd(),AvgStd(),AvgStd()};
float sensorLastReading[12] = {-127,-127,-127,-127,-127,-127,
                               -127,-127,-127,-127,-127,-127};
bool relayState[8] = {false,false,false,false,false,false,false,false};
int volatile _optionValueChange = 0;

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

  // Begin I2C channels
  I2C_PRIMARY.begin(I2C_1_SDA, I2C_1_SCL, 400000ul);
  I2C_SECONDARY.begin(I2C_2_SDA, I2C_2_SCL, 400000ul);
  
  initRelayOutputs(); //Doing them first to avoid having them floating
  initEncoderInput();
  initDisplays();

  Serial.println("Checking EEPPROM....");
  if(_eepromController.init(&I2C_PRIMARY)){
    Serial.println("EEPPROM initializied sucessfully!");
  }
  else
  {
    Serial.println("EEPPROM failed to initialize!");
  }
  
  
  // Sensor V2 - reboot issues 

  Serial.println("Init Sensor Controller....");
  // ONE_WIRE_BUS_A IS CAUSING the ESP32 to crash...maybe a short? 
  //_sensorController.init(channelsConfig, &oneWireBusA, &oneWireBusB);
  _sensorController.init(channelsConfig, &oneWireBusA, &oneWireBusB);

  Serial.print("Sensor Count BusA: ");
  Serial.println(_sensorController.sensorCountPerBus(0));
  Serial.print("Sensor Count BusB: ");
  Serial.println(_sensorController.sensorCountPerBus(1));


  /*
  // Sensor V1
  pinMode(ONE_WIRE_BUS_A, INPUT);
  //pinMode(ONE_WIRE_BUS_B, INPUT); //INPUT_PULLUP
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
  if(sensors.getAddress(address, 0)){ // sometimes device count doesn't work when OneWire HIGH voltage is low (like 3v, instead of 3.3)
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
  // Sensor 1 END
  */
  
  // Address stuff END 
  
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
  xTaskCreatePinnedToCore(TaskReadTempSensors, "ReadTempSensors", 32768, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  xTaskCreatePinnedToCore(TaskUpdateRelays, "UpdateRelays", 32768, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  
  // Now the task scheduler, which takes over control of scheduling individual tasks, is automatically started.
}

void loop()
{
  // Empty. Things are done in Tasks.
}

void initDisplays(){
  Serial.println("Init display starting...");
  
  // Init Main Display
  mainDisplay.init(&I2C_PRIMARY);

  // Init Slave Displays
  slaveDisplay[0].init(0, &I2C_SECONDARY);
  slaveDisplay[1].init(1, &I2C_SECONDARY);
  slaveDisplay[2].init(2, &I2C_SECONDARY);
  
  Serial.println("Init display finshed!");
}

void initEncoderInput(){
  Serial.println("Init Encoder...");
  pinMode(ENCODER_CLK, INPUT); // Pull up done on the encoder already
  pinMode(ENCODER_DT, INPUT); // Pull up done on the encoder already
  pinMode(ENCODER_BTN, INPUT_PULLUP);
    
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
    
    //Serial.println("<MAIN:TaskReadTempSensors> Request Temps");
    //sensors.requestTemperatures();
    _sensorController.requestTemperatures(); //  sensor v2

    // Display temperature from each sensor
    for (int i = 0;  i < SENSORS_MAX ;  i++)
    {
      //if(SENSOR_ADDRESS_ACTIVE[i]){
        
        
        // Sensor V2
        float tempC = _sensorController.getTempByChannel(i);
        sensorLastReading[i] = tempC;
        if(tempC > -50)
        {
          sensorStat[i].checkAndAddReading(tempC);
        }
        
        /* // Sensor V1
        float tempC = sensors.getTempC(SENSOR_ADDRESS_ACTIVE[i]);
        if(tempC == DEVICE_DISCONNECTED_C){
          sensorLastReading[i] = SENSOR_ERROR; // Using -128 to 
        }else{
          sensorLastReading[i] = tempC;
          sensorStat[i].checkAndAddReading(tempC);
        } */
      //}
      
    }
    vTaskDelay(1000);  // one tick delay (15ms) in between reads for stability
  }
}

void TaskUpdateMainDisplay(void *pvParameters) 
{
  (void) pvParameters;
  
  for (;;)
  {
    if(mainDisplay.IN_OPTIONS_MENU){
      mainDisplay.renderOptionsMenu();
    }
    else
    {
      mainDisplay.setSlavesPage(CHANNEL_BANK);
      mainDisplay.renderPage(CURRENT_PAGE);
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
    if(mainDisplay.IN_OPTIONS_MENU)
    { 
      if(mainDisplay.IN_OPTIONS_PAGE == 1) // Channel config
      {
        //slaveDisplay[0].renderText(convertAddress(SENSOR_ADDRESS_ACTIVE[0]), mainDisplay.IN_OPTIONS_SELECTOR == 1); // POSIBLY CAUSING A CRASH
        slaveDisplay[0].renderText("TENP SENSOR ADDR", mainDisplay.IN_OPTIONS_SELECTOR == 1); // POSIBLY CAUSING A CRASH
        slaveDisplay[1].renderOption("Target:", 17.2, mainDisplay.IN_OPTIONS_SELECTOR == 2);
        slaveDisplay[2].renderOption("Tolerance:      +/-", 0.5, mainDisplay.IN_OPTIONS_SELECTOR == 3);
      }
      else if(mainDisplay.IN_OPTIONS_PAGE == 2) // Output binding
      {
        slaveDisplay[0].renderOption("Output:", mainDisplay.IN_OPTIONS_SELECTOR, mainDisplay.IN_OPTIONS_SELECTOR == 2, true);
        slaveDisplay[1].renderText("", false);
        slaveDisplay[2].renderText("", false);
      }


    }
    else
    {
      if(!CHANNEL_BANK){
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
    }
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
      if(relayMapping[i] < 8)
      {
        ChannelConfig *chConfig = &channelsConfig[relayMapping[i]];
        
        if(sensorLastReading[relayMapping[i]] > -127)
        {
          if(sensorLastReading[relayMapping[i]] >= ((*chConfig).target + (*chConfig).tolerance)) // If reached High Threshold
          {  
            relayState[i] = true;
            digitalWrite(RelayPins[i], false); 
          }
          else if(sensorLastReading[relayMapping[i]] <= ((*chConfig).target - (*chConfig).tolerance))
          {
            relayState[i] = false;
            digitalWrite(RelayPins[i], true); 
          }
        }
      }
      else
      {
        relayState[i] = false;
        digitalWrite(RelayPins[i], true); 
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

    // ------- INCREASE --------
    if( encval > 3) {        // Four steps forward
      if(!mainDisplay.IN_OPTIONS_MENU){
        if(CURRENT_PAGE < MENU_PAGE_MAX){
          CURRENT_PAGE++;  
        }
      }
      else
      { 
        switch (mainDisplay.IN_OPTIONS_PAGE)
        {
          case 1:
            if(!mainDisplay.IN_OPTIONS_SELECTOR_LOCKED)
            {
              if(mainDisplay.IN_OPTIONS_SELECTOR == OPTIONSCONFIG_SELECTOR_MAX)
              {
                mainDisplay.IN_OPTIONS_SELECTOR = 0;
              }
              else
              {
                mainDisplay.IN_OPTIONS_SELECTOR++;
              }
            }
            else
            {
              _optionValueChange++;
            }
            break;
          case 2:
            if(!mainDisplay.IN_OPTIONS_SELECTOR_LOCKED)
            {
              if(mainDisplay.IN_OPTIONS_SELECTOR == OPTIONSCONFIG_RELAY_SELECTOR_MAX)
              {
                mainDisplay.IN_OPTIONS_SELECTOR = 0;
              }
              else
              {
                mainDisplay.IN_OPTIONS_SELECTOR++;
              }
            }
            else
            {
              _optionValueChange++;
            }
            
            break;
          default:
            if(mainDisplay.IN_OPTIONS_SELECTOR < OPTIONS_SELECTOR_MAX)
            {
              mainDisplay.IN_OPTIONS_SELECTOR++;
            }
            break;
        }
      }
      
      encval = 0;
    }
    // ------- DECREASE --------
    else if( encval < -3 ) 
    {  // Four steps backwards
      if(!mainDisplay.IN_OPTIONS_MENU){
        if(CURRENT_PAGE!=0){
          CURRENT_PAGE--;// Decrease counter
        } 
      }
      else
      {
        switch (mainDisplay.IN_OPTIONS_PAGE)
        {
          case 1:
            if(!mainDisplay.IN_OPTIONS_SELECTOR_LOCKED)
            {
              if(mainDisplay.IN_OPTIONS_SELECTOR == 0)
              {
                mainDisplay.IN_OPTIONS_SELECTOR = OPTIONSCONFIG_SELECTOR_MAX;
              }
              else
              {
                mainDisplay.IN_OPTIONS_SELECTOR--;
              }
            }
            else
            {
              _optionValueChange--;
            }
            break;
          case 2:
            if(!mainDisplay.IN_OPTIONS_SELECTOR_LOCKED)
            {
              if(mainDisplay.IN_OPTIONS_SELECTOR == 0)
              {
                mainDisplay.IN_OPTIONS_SELECTOR = OPTIONSCONFIG_RELAY_SELECTOR_MAX;
              }
              else
              {
                mainDisplay.IN_OPTIONS_SELECTOR--;
              }
            }
            else
            {
              _optionValueChange--;
            }
            
            break;
          default:
            if(mainDisplay.IN_OPTIONS_SELECTOR > 0)
            {
              mainDisplay.IN_OPTIONS_SELECTOR--;
            }
            break;
        }
      }
     encval = 0;
    }
}

// All of this code could probably be in the MainDisplayHandler class, but leaving it here for now
void IRAM_ATTR ISR_RotaryBTN(void)
{
  // NEED DEBOUNCER Check of like 30ms minimum
    noInterrupts();
    if(xTaskGetTickCountFromISR() - lastBTNPressTickCount > 30){
      if(!mainDisplay.IN_OPTIONS_MENU)
      {
        mainDisplay.IN_OPTIONS_MENU = true;
      }
      else
      {
        switch (mainDisplay.IN_OPTIONS_PAGE)
        {
          case 0:
            {
              switch (mainDisplay.IN_OPTIONS_SELECTOR)
              {
                case 0: // Channel Config
                  mainDisplay.IN_OPTIONS_PAGE = 1; 
                  mainDisplay.IN_OPTIONS_SELECTOR = 0;
                  break;
                case 1: // Output Relay Mapping 
                  mainDisplay.IN_OPTIONS_PAGE = 2;
                  mainDisplay.IN_OPTIONS_SELECTOR = 0;
                  break;
                case 2: // Switch Bank 
                  mainDisplay.IN_OPTIONS_PAGE = 0;
                  CHANNEL_BANK = !CHANNEL_BANK;
                  mainDisplay.IN_OPTIONS_MENU = false;
                  break;
                case 3: // EXIT
                  mainDisplay.IN_OPTIONS_PAGE = 0;
                  mainDisplay.IN_OPTIONS_SELECTOR = 0;
                  mainDisplay.IN_OPTIONS_MENU = false;
                  break;
              }
            }
            break;
          case 1:
            mainDisplay.IN_OPTIONS_PAGE = 0;
            mainDisplay.IN_OPTIONS_SELECTOR = 0;
            mainDisplay.IN_OPTIONS_MENU = false;
            break;
          case 2:
            {
              if(mainDisplay.IN_OPTIONS_SELECTOR_LOCKED)
              {
                mainDisplay.IN_OPTIONS_SELECTOR_LOCKED = false;
              }
              else
              {
                if(mainDisplay.IN_OPTIONS_SELECTOR == 12 || mainDisplay.IN_OPTIONS_SELECTOR == 13)
                {
                  mainDisplay.IN_OPTIONS_PAGE = 0;
                  mainDisplay.IN_OPTIONS_SELECTOR = 0;
                  mainDisplay.IN_OPTIONS_MENU = false;
                }
                else
                {
                  mainDisplay.IN_OPTIONS_SELECTOR_LOCKED = true;
                }
              }
            }
            break;
        }
      }
    }
    lastBTNPressTickCount = xTaskGetTickCountFromISR();
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

String convertAddress(DeviceAddress deviceAddress)
{
  //   {0x28,0xA0,0xF4,0x79,0xA2,0x00,0x03,0x9B}, //(1) 28A0 F479 A200 039B 
  String val = "";
  for (uint8_t i = 0; i < 8; i++)
  {
    if(deviceAddress[i] < 10)
    {
      val += "0" + String(deviceAddress[i],HEX);
    }
    else
    {
      String temp = String(deviceAddress[i],HEX);
      temp.toUpperCase();
      val += temp;
    }
    if(i%2 && i>0){
      val += " ";
    }
  }
  return val;
}
