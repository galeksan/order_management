#include <iostream>
#include "OrderManagement.h"
#include "OrderStatsCollector.h"
#include "Config.h"
#include "ExchangeSimulator.h"
#include "MockOrdersGenerator.h"
#include <chrono>
#include <iostream>
#include <inttypes.h>

using namespace ordermanagement;

void test1()
// Open and close times are read from config
{
    std::unique_ptr<IOrderStatsCollectorCallBack> callBack = 
        std::make_unique<OrderStatsFileWriterCallback>("test1.txt");
    std::string configFilename = "../config/config.txt";
    OrderManagement manager(configFilename, std::move(callBack));
    ExchangeResponseSimulator simulator(&manager);
    manager.setExchangeSimulator(&simulator);
    manager.start();
    MockOrdersGenerator client(&manager, 1);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Terminating 1" << std::endl;
}

void test2()
// set open time 2 seconds after current time, close time 8 seconds after
// and check that orders are first rejected then accepted
// then rejected again as exchange closes
{
    std::unique_ptr<IOrderStatsCollectorCallBack> callBack = 
        std::make_unique<OrderStatsFileWriterCallback>("test2.txt");
    std::string configFilename = "../config/config.txt";
    OrderManagement manager(configFilename, std::move(callBack));
    Config& config = manager.getConfig();
    uint64_t currentTime = getCurrentTimeNs();
    config.dumpConfig();
    const auto currentTimeOffsetFromDateStart = currentTime % NS_IN_DAY;
    config.openTimeOffsetFromDayStartNs = currentTimeOffsetFromDateStart + 2 * NS_IN_SECOND;
    config.closeTimeOffsetFromDayStartNs = currentTimeOffsetFromDateStart + 8 * NS_IN_SECOND;
    config.dumpConfig();
    ExchangeResponseSimulator simulator(&manager);
    manager.setExchangeSimulator(&simulator);
    manager.start();
    MockOrdersGenerator client(&manager, 1);
    std::this_thread::sleep_for(std::chrono::seconds(12));
    std::cout << "Terminating 2" << std::endl;
}

void test3()
// 3 clients, simultaneously submitting orders to OrderManagement
{
    std::unique_ptr<IOrderStatsCollectorCallBack> callBack = 
        std::make_unique<OrderStatsFileWriterCallback>("test3.txt");
    std::string configFilename = "../config/config.txt";
    OrderManagement manager(configFilename, std::move(callBack));
    Config& config = manager.getConfig();
    uint64_t currentTime = getCurrentTimeNs();
    const auto currentTimeOffsetFromDateStart = currentTime % NS_IN_DAY;
    config.openTimeOffsetFromDayStartNs = currentTimeOffsetFromDateStart - 5 * NS_IN_SECOND;
    config.closeTimeOffsetFromDayStartNs = currentTimeOffsetFromDateStart + 30 * NS_IN_SECOND;
    config.dumpConfig();
    ExchangeResponseSimulator simulator(&manager);
    manager.setExchangeSimulator(&simulator);
    manager.start();
    MockOrdersGenerator client1(&manager, 1);
    MockOrdersGenerator client2(&manager, 2);
    MockOrdersGenerator client3(&manager, 3);
    std::this_thread::sleep_for(std::chrono::seconds(12));
    std::cout << "Terminating 3" << std::endl;
}

int main(int, char**)
{
    
    // test1 reads open/close times from the config file,
    // so in order to see an output in the test1.txt 
    // we need to set up correct times in the config.
    // test1(); 
    
    
    // Each test case below will produce one output file 
    // with order latency information. 
    // I use dependency injection in the design to perform the testing.
    test2();
    test3();
    return 0;
}