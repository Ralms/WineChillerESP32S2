
#include "EEPROMController.h"
#include "ChannelConfig.h"

class ConfigController{
    public:
        ConfigController();
        void init(EEPROMController eeprom);
        ChannelConfig activeChannel; // Channel under configuration
        bool saveChannel(ChannelConfig channel);
        bool loadChannelConfigs(ChannelConfig* channelsArray);
    private:


};