// Provides basic config file parsing functionality to read configuration parameters from a file.
// Config file supports trading open/close times in HH:MM:SS am/pm format
// (to represent this in the code, I use offsets from current day start).
// Config file also supports throttling rate, sliding window size in seconds,
// username/password parameter to login to the exchange.
// I use simple paramName=paramValue format in the config, as I am not allowed
// to use third party libraries to work with more widespread config file formats (e.g. XML).
// I am also assuming that the time range covered by open/close times does not cross the midnight
// in UTC timezone (i.e. trading can't open at 9pm today and close at 4am tomorrow,
// however it can open at 9am today and close at 4pm today).
// I understand that there are exchanges that trade 23 hours a day (e.g. CME), but for the sake of
// simplicity I am doing this assumption.
// Sample config file can be found in ordermanagement/config/config.txt file

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <fstream>
#include <sstream>

namespace ordermanagement {
    
struct Config {
    explicit Config(const std::string& configFileName);
    void dumpConfig() const;

    uint64_t openTimeOffsetFromDayStartNs;
    uint64_t closeTimeOffsetFromDayStartNs;
    uint32_t windowSizeSec;
    uint32_t throttlingRate;
    std::string username;
    std::string password;
};

}
// ordermanagement namespace

#endif