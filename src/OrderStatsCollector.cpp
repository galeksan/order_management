#include <iostream>
#include <string>

#include "OrderStatsCollector.h"

namespace ordermanagement {

OrderStatsFileWriterCallback::OrderStatsFileWriterCallback(const std::string& filename) 
    : m_StatsFile(filename)
{
    m_StatsFile << "#OrderId,ResponseType,OrderWaitTimeInQueue,OrderRoundTripLatency\n";
}

void OrderStatsFileWriterCallback::processOrderStatisticsInfo(OrderResponse && response, 
                                                              const OrderStats& orderStats)
{
    m_StatsFile << response << "," << orderStats << "\n";
}

}  // ordermangement namespace