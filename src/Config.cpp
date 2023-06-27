#include "Config.h"
#include "iostream"
#include <vector>
#include <unordered_map>

namespace ordermanagement {

namespace {
uint64_t getTime(std::string time) 
{
    bool amFlag = false;
    if(time.find("am") == std::string::npos && time.find("pm") == std::string::npos) {
        throw std::runtime_error("Invalid config, missing am or pm");
    } else {
        auto pos = time.find("am");
        if(pos != std::string::npos) {
            time = time.substr(0, pos);
            amFlag = true;
        } else {
            time = time.substr(0, time.find("pm"));
        }
    }
    std::istringstream iss{time};
    std::vector<std::string> tokens;
    std::string token;
    while (std::getline(iss, token, ':')) {
        tokens.emplace_back(token);
    }

    int hours = std::stoi(tokens[0]);
    int mins = std::stoi(tokens[1]);
    int secs = std::stoi(tokens[2]);
    if(!amFlag) {
        hours += 12;
    }
    uint64_t offset = 1000000000ul * 
        (hours * 60 * 60 + mins * 60 + secs);
    return offset;
}
} // unnamend namespace

Config::Config(const std::string& configFileName)
{
    std::ifstream ifs(configFileName);
    std::string line;
    std::unordered_map<std::string, std::string> params;
    while(std::getline(ifs, line)) {
        if(!line.empty() && line[0] != '#') {
            auto pos = line.find('=');
            if(pos == std::string::npos) {
                throw std::runtime_error("Parser Error");
            }
            std::string paramName = line.substr(0, pos);
            std::string paramVal = line.substr(pos + 1);
            params.emplace(paramName, paramVal);
        }
    }
    openTimeOffsetFromDayStartNs = getTime(params["Open"]);
    closeTimeOffsetFromDayStartNs = getTime(params["Close"]);
    windowSizeSec = std::stoul(params["MonitorWindowSec"]);
    throttlingRate = std::stoul(params["Rate"]);
    username = params["Username"];
    password = params["Password"];
}

void Config::dumpConfig() const
{
    std::cout << "openTimeOffsetFromDayStartNs=" << openTimeOffsetFromDayStartNs << "\n"
              << "closeTimeOffsetFromDayStartNs=" << closeTimeOffsetFromDayStartNs << "\n"
              << "windowSizeSec=" << windowSizeSec << "\n"
              << "throttlingRate=" << throttlingRate << "\n"
              << "username=" << username << "\n"
              << "password=" << password << "\n";
}

} // ordermangement namespace