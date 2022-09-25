#include "Arduino.h"

class ChannelConfig {
    public:
        ChannelConfig(int _channelIdx, uint8_t _sensorAddress[8], float _target = 17.0, float _tolerance = 0.5){
            channelIdx = _channelIdx;
            sensorAddress = _sensorAddress;
            target = _target;
            tolerance = _tolerance;
        };
        uint8_t channelIdx;
        uint8_t *sensorAddress;
        float target = 17.0;
        float tolerance = 0.5;
};