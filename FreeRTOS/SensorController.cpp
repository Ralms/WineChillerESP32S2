#include "SensorController.h"


SensorController::SensorController(){ }

bool SensorController::init(ChannelConfig sensorConfigMapping[12], OneWire *busA){
    Serial.println("<SensorController> Init DallasTemperatures... ");

    initBus(busA, 0);

    delay(200);
    Serial.println("<SensorController> Buses initialized. ");

    // Update Sensor Count for each Bus
    for(int i = 0; i < 4; ++i) 
    {
        if(sensorBusEnabled[i])
        {
            sensorCount[i] = sensors[i].getDeviceCount();
        }
    }
    Serial.println("<SensorController> Sensor Count per Bus set.");

    // Init Sensor Config Channel mapping

    refreshConfigMapping(sensorConfigMapping);

    Serial.println("<SensorController> Init Complete!");
    return 0;
}

bool SensorController::init(ChannelConfig sensorConfigMapping[12], OneWire *busA, OneWire *busB){
    Serial.println("<SensorController> Init DallasTemperatures... ");

    initBus(busA, 0);
    initBus(busB, 1);

    delay(200);
    Serial.println("<SensorController> Buses initialized. ");

    // Update Sensor Count for each Bus
    for(int i = 0; i < 4; ++i) 
    {
        if(sensorBusEnabled[i])
        {
            sensorCount[i] = sensors[i].getDeviceCount();
        }
    }
    Serial.println("<SensorController> Sensor Count per Bus set.");

    // Init Sensor Config Channel mapping

    refreshConfigMapping(sensorConfigMapping);

    Serial.println("<SensorController> Init Complete!");
    return 0;
}

bool SensorController::init(ChannelConfig sensorConfigMapping[12], OneWire *busA, OneWire *busB, OneWire *busC, OneWire *busD){

    
    // Init DallasTemperatures
    /*
    if(busA_Pin > 0)
    {
        OneWire oneWireBusA = OneWire(busA_Pin);
        sensors[0] = DallasTemperature(&oneWireBusA);

        sensors[0].begin();
        delay(1000);
        sensors[0].setResolution(12); // Set resolution to 12 bit
        sensors[0].requestTemperatures();
        sensorBusEnabled[0] = true;
        Serial.println("<SensorController> Bus A set. ");
    }

    if(busB_Pin > 0)
    {
        OneWire oneWireBusB = OneWire(busB_Pin);
        sensors[1] = DallasTemperature(&oneWireBusB);

        sensors[1].begin();
        delay(1000);
        sensors[1].setResolution(12); // Set resolution to 12 bit
        sensors[1].requestTemperatures();
        sensorBusEnabled[1] = true;
        Serial.println("<SensorController> Bus B set. ");
    }

    if(busC_Pin > 0)
    {
        OneWire oneWireBusC = OneWire(busC_Pin);
        sensors[2] = DallasTemperature(&oneWireBusC);

        sensors[2].begin();
        delay(1000);
        sensors[2].setResolution(12); // Set resolution to 12 bit
        sensors[2].requestTemperatures();
        sensorBusEnabled[2] = true;
        Serial.println("<SensorController> Bus C set. ");
    }

    if(busD_Pin > 0)
    {
        OneWire oneWireBusD = OneWire(busD_Pin);
        sensors[3] = DallasTemperature(&oneWireBusD);

        sensors[3].begin();
        delay(1000);
        sensors[3].setResolution(12); // Set resolution to 12 bit
        sensors[3].requestTemperatures();
        sensorBusEnabled[3] = true;
        Serial.println("<SensorController> Bus D set. ");
    }
    */

    delay(200);
    Serial.println("<SensorController> Buses initialized. ");

    // Update Sensor Count for each Bus
    for(int i = 0; i < 4; ++i) 
    {
        if(sensorBusEnabled[i])
        {
            sensorCount[i] = sensors[i].getDeviceCount();
        }
    }
    Serial.println("<SensorController> Sensor Count per Bus set.");

    // Init Sensor Config Channel mapping

    refreshConfigMapping(sensorConfigMapping);

    Serial.println("<SensorController> Init Complete!");
    return 0;
}

// PRIVATE
void SensorController::initBus(OneWire *busA, int idx){
    sensors[idx] = DallasTemperature(busA);
    sensors[idx].begin();
    delay(1000);
    sensors[idx].setResolution(12); // Set resolution to 12 bit
    sensors[idx].requestTemperatures();
    sensorBusEnabled[idx] = true;
    Serial.println("<SensorController> Bus " + String(idx) + " set. ");
}

bool SensorController::refreshConfigMapping(ChannelConfig sensorConfigMapping[12]){

    Serial.println("<SensorController> refreshConfigMapping ... ");

    //bool mappedSensors[40]; 

    for(int i = 0; i < 12; ++i)  // Cycle all config channel mapping
    {
        if(sensorConfigMapping[i].sensorAddress != 0)
        {
            for(int k = 0; k < 4; ++k) // Check all 4 banks
            {
                if(sensorBusEnabled[k] && sensors[k].isConnected(sensorConfigMapping[i].sensorAddress))
                {
                    Serial.print("<SensorController> Channel ");
                    Serial.print(i);
                    Serial.print(" sensor connected on Bus ");
                    Serial.println(k);
                    // Not sure how to map between Banks
                    channelEnabled[i] = true;
                    channelMapping[i] = sensorConfigMapping[i].sensorAddress;
                    channelBank[i] = &sensors[k];

                    /*
                    for(u_int8_t j = 0; j < sensors[k].getDeviceCount(); ++j)
                    {
                        DeviceAddress deviceAddress;
                        if(sensors[k].getAddress(deviceAddress, j))
                        {
                            mappedSensors[(k*j) + j] = true;
                        }
                    }
                    */
                }
            }
        }
    }

    return 0;
}

uint16_t SensorController::sensorCountPerBus(uint16_t busIndex){
    return sensorCount[busIndex];
}

float SensorController::getTempByChannel(int channelIndex){
    if(channelEnabled[channelIndex]){
        float read = (*channelBank[channelIndex]).getTempC(channelMapping[channelIndex]);
        if(read == DEVICE_DISCONNECTED_C)
        {
            return -128; // SENSOR_ERROR
        }
        return read;
    }
    else
    {
        return -127;
    };
    return -127;
}

void SensorController::requestTemperatures(){
    for(int i = 0; i < 4; ++i) // Check all 4 banks
    {
        if(sensorBusEnabled[i])
        {
            sensors[i].requestTemperatures();;
        }
    }   
}