// OrderStatsFileWriterCallback/OrderStatsCollector classes 
// IOrderStatsCollectorCallBack provides abstract interface to receive order stats information.
// Clients can implement this interface to receive order stats information. As exercise requires us
// to only write collected stats in persistent storage, OrderStatsFileWriterCallback class
// implements this interface and writes received latency information to a file.
// Note that if other components/classes in the trading system, needed to receive order stats information,
// we could easily change the design to register listeners and broadcast this information to other interested
// parties.

                                                           
#ifndef ORDER_STATS_COLLECTOR_H
#define ORDER_STATS_COLLECTOR_H

#include <fstream>
#include "Utils.h"

namespace ordermanagement {

class IOrderStatsCollectorCallBack {
public:
    virtual ~IOrderStatsCollectorCallBack() {}
    virtual void processOrderStatisticsInfo(OrderResponse && response, 
                                            const OrderStats& orderInfo) = 0;
};

class OrderStatsFileWriterCallback : public IOrderStatsCollectorCallBack {  
public:
    ~OrderStatsFileWriterCallback() { m_StatsFile.close(); }
    OrderStatsFileWriterCallback(const std::string& filename);
    void processOrderStatisticsInfo(OrderResponse && response,
                                    const OrderStats& orderInfo) override;
private:
    std::ofstream m_StatsFile;
};

} // ordermanagement namespace

#endif