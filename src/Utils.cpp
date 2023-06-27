#include <chrono>
#include <iostream>
#include "Utils.h"

namespace ordermanagement {

std::ostream& operator<<(std::ostream& ofs, const OrderStats& stats)
{
    int64_t waitTimeInQueue = stats.requestSendTimeNs - stats.orderManagerReceiveTimeNs;
    int64_t roundTripLatency = stats.responseReceivalTimeNs - stats.requestSendTimeNs;
    ofs << waitTimeInQueue << "," << roundTripLatency;
    return ofs;
}

std::ostream& operator<<(std::ostream& ofs, const OrderResponse& response)
{
    ofs << response.orderId << "," << static_cast<int>(response.responseType);
    return ofs;
}

std::uint64_t getCurrentTimeNs()
{
    auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>
        (std::chrono::high_resolution_clock::now());
    return now.time_since_epoch().count();
}

} // ordermanagement namespace