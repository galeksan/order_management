// This is the main class that performs order management and exchange transmission rate limiting.
// On a high level there are 2 threads working with orders, one that writes orders to a queue
// and another one that reads from the queue and transmits orders to the exchange, with appropriate
// pre configured rate limiting. There is also third thread that periodically checks the system time
// to send login/logout requests when needed and sets and sets m_exchangeOpen flag,
// indicating whether orders need to be processed or rejected by OrderManagement.
// Note that this thread uses relatively long sleeps when exchange open/close times are far away
// from the current time, however when exchange open/close times get closer to the current time,
// this thread performs a busy wait, to start transmitting orders instantly when exchange opens up.
// Given the requirements of this exercise, I think there is no need for us to keep orders in the
// queue once they have been transmitted to the exchange. This is because we only care about
// order statistics information after order was transmitted to the exchange, so we can just save
// per order stats in some data structure after order transmission to match it with its response 
// when it arrives, to calculate order roundTrip latency and other stats.
// I think that order wait time in a queue is also an interested indicator that worth reporting.
// It can also be a nice indicator of overall system performance and potential bottlnecks.
// That's why I calculate 2 latency stats per order, order wait time in the queue and
// time between order transmission and its response receival.
// I also slightly modified one of the onData functions declaration, please see the comment above it
// for the reasoning behind that design decision.


#ifndef ORDER_MANAGEMENT_H
#define ORDER_MANAGEMENT_H

class OrderRequest;
class OrderResponse;

#include <atomic>
#include <queue>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <functional>

#include "Config.h"
#include "Utils.h"
#include "OrderStatsCollector.h"

namespace ordermanagement {

class IExchangeSimulator;

class OrderManagement
{
public:    
    OrderManagement(const std::string& configFileName, 
                    std::unique_ptr<IOrderStatsCollectorCallBack> statsCollector);
    
    void start();
    void shutDown();
    ~OrderManagement();

    // These 2 functions will be used for testing
    Config& getConfig() { return m_config; }
    void setExchangeSimulator(IExchangeSimulator* simulator);

    // Please note that I slightly modified the onData function declaration here to accept RequestType. 
    // The alternative would be to make RequestType member of OrderRequest, but that would mean that 
    // exchange should support modifications. As it wasn't provided in OrderRequest definition I decided
    // to make it as a function parameter and don't send modify requests to the exchange 
    // (rather support modifications of orders when they are still in our system)
    // I assume this function can be called by multiple upstream threads.
    void onData(OrderRequest && request, RequestType requestType);

    void onData(OrderResponse && response);
    void send(const OrderRequest& request);

    // Sends the logon message to exchange.
    void sendLogon();

    // Sends the logout message to exchange.
    void sendLogout();

private:
    static constexpr uint64_t REGULAR_SLEEP_TIME_NS = 1000000ull;
    static constexpr uint64_t SHORT_SLEEP_TIME_NS = 1ull; // 1 nano
    void checkExchangeState();
    void waitOrAct(std::function<void()> act, 
                   uint64_t currentTimeOffsetFromDateStart,
                   uint64_t actionTimeOffsetFromDateStart);
    void rejectOrder(OrderRequest && request, const std::string rejectReason);
    void addRequestToQueue(OrderRequest && request);
    void transmitRemoteRequests();
    void rejectOrdersInQueue(const std::string& rejectReason);
    bool transmitOneOrder(uint64_t& sendTime);

private:
    std::atomic_bool m_exchangeOpen = false;
    std::atomic_bool m_terminate = false;
    
    Config m_config;
    
    std::mutex m_ordersQueueMutex;
    std::mutex m_ordersStatsMutex;

    // it will use deque as underlined structure by default
    std::queue<OrderInfo> m_ordersQueue;
    std::unordered_map<uint64_t, OrderInfo*> m_queuedOrdersMap;
    std::unordered_map<uint64_t, OrderStats> m_ordersStatsMap;
    
    std::unique_ptr<std::thread> m_checkExchangeState;
    std::unique_ptr<std::thread> m_transmitRemoteRequests;

    std::unique_ptr<IOrderStatsCollectorCallBack> m_statsCollector;
    IExchangeSimulator* m_simulator;
};

} // ordermanagement namespace

#endif