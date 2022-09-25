#include "Arduino.h"
#include "ChannelConfig.h"
#include <OneWire.h>
#include <DallasTemperature.h>

class SensorController {

    public:
        SensorController();
        bool init(ChannelConfig sensorConfigMapping[12], OneWire *busA);
        bool init(ChannelConfig sensorConfigMapping[12], OneWire *busA, OneWire *busB);
        bool init(ChannelConfig sensorConfigMapping[12], OneWire *busA, OneWire *busB, OneWire *busC, OneWire *busD);
        bool refreshConfigMapping(ChannelConfig sensorConfigMapping[12]);
        uint16_t sensorCountPerBus(uint16_t busIndex); // Bus A = 1, B = 2, etc. 
        float getTempByChannel(int channelIndex);

        void requestTemperatures(); // DallasTemperature::requestTemperatures()
    private:
        void initBus(OneWire *busA, int idx = 0);
        uint16_t sensorCount[4] = {0,0,0,0};
        DallasTemperature sensors[4];
        bool sensorBusEnabled[4] = {false, false, false, false};
        uint8_t *channelMapping[12];
        bool channelEnabled[12];
        DallasTemperature *channelBank[12];
};