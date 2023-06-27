#include <chrono>
#include <iostream>
#include <queue>
#include <functional>

#include "OrderManagement.h"
#include "ExchangeSimulator.h"
#include "Config.h"

namespace ordermanagement {

OrderManagement::OrderManagement(const std::string& configFileName,
                  std::unique_ptr<IOrderStatsCollectorCallBack> statsCollector)
    : m_config(configFileName)
    , m_statsCollector(std::move(statsCollector))
{
}

void OrderManagement::start()
{
    m_checkExchangeState = std::make_unique<std::thread>(
        &OrderManagement::checkExchangeState, this);
    m_transmitRemoteRequests = std::make_unique<std::thread>(
        &OrderManagement::transmitRemoteRequests, this);
}

void OrderManagement::shutDown()
{
    m_terminate = true;
}

OrderManagement::~OrderManagement()
{
    shutDown();
    m_checkExchangeState->join();
    m_transmitRemoteRequests->join();
    rejectOrdersInQueue("Terminate has been called");
}

void OrderManagement::setExchangeSimulator(IExchangeSimulator* simulator)
{
    m_simulator = simulator;
}

void OrderManagement::onData(OrderRequest && request, RequestType requestType)
{
    if (!m_exchangeOpen) {
        rejectOrder(std::move(request), "Exchange is closed");
    } else {
        switch (requestType) {
            case RequestType::Unknown:
                rejectOrder(std::move(request), "Unknown request type");
                break;
            case RequestType::New:
                addRequestToQueue(std::move(request));
                break;
            case RequestType::Modify: {
                    std::lock_guard<std::mutex> lock(m_ordersQueueMutex);
                    auto orderIt = m_queuedOrdersMap.find(request.orderId);
                    if(orderIt != m_queuedOrdersMap.end()) {
                        orderIt->second->request = std::move(request);
                    } else {
                        std::cerr << "Can't modify order it has already been sent to the exchange\n";
                    }
                }
                break;
            case RequestType::Cancel: {
                    std::lock_guard<std::mutex> lock(m_ordersQueueMutex);
                    auto orderIt = m_queuedOrdersMap.find(request.orderId);
                    if(orderIt != m_queuedOrdersMap.end()) {
                        orderIt->second->canceledFlag = true;
                    } else {
                        std::cerr << "Can't cancel order as it has already been submitted to the exchange\n";
                    }
                }
                break;
        }
    }
}

void OrderManagement::onData(OrderResponse && response)
{
    uint64_t currentTime = getCurrentTimeNs();
    std::lock_guard<std::mutex> lock(m_ordersStatsMutex);
    auto orderStatIt = m_ordersStatsMap.find(response.orderId);
    orderStatIt->second.responseReceivalTimeNs = currentTime;
    auto orderId = response.orderId;
    m_statsCollector->processOrderStatisticsInfo(std::move(response), orderStatIt->second);
    m_ordersStatsMap.erase(orderId);
}

void OrderManagement::send(const OrderRequest& request)
{
    m_simulator->send(request);
}

void OrderManagement::sendLogon()
{
    m_simulator->sendLogon(Logon{m_config.username, m_config.password});
}

void OrderManagement::sendLogout()
{
    m_simulator->sendLogout(Logout{m_config.username});
}

void OrderManagement::checkExchangeState()
{
    while (!m_terminate) {
        uint64_t currentTime = getCurrentTimeNs();
        const auto currentTimeOffsetFromDateStart = currentTime % NS_IN_DAY;
        if (currentTimeOffsetFromDateStart >= m_config.closeTimeOffsetFromDayStartNs) {
            std::this_thread::sleep_for(std::chrono::nanoseconds(REGULAR_SLEEP_TIME_NS));
        } else {
            if (!m_exchangeOpen) {
                waitOrAct([this]() { sendLogon(); }
                    , currentTimeOffsetFromDateStart
                    , m_config.openTimeOffsetFromDayStartNs);
            } else {
                constexpr uint64_t THRESHOLD_NS = 10;
                // Mark the exchange as closed 10 nanoseconds before the close time
                // to not send orders after close
                waitOrAct([this](){ sendLogout(); }
                    , currentTimeOffsetFromDateStart
                    , m_config.closeTimeOffsetFromDayStartNs - THRESHOLD_NS);
            }
        }      
    }
}

void OrderManagement::waitOrAct(std::function<void()> act, 
                                uint64_t currentTimeOffsetFromDateStart,
                                uint64_t actionTimeOffsetFromDateStart)
{
    if(currentTimeOffsetFromDateStart < actionTimeOffsetFromDateStart 
        && actionTimeOffsetFromDateStart - currentTimeOffsetFromDateStart > 3 * REGULAR_SLEEP_TIME_NS) {
        std::this_thread::sleep_for(std::chrono::nanoseconds(REGULAR_SLEEP_TIME_NS));
    } else { // we are close to the trading Open/Close time, perform busy check of the time to avoid unnessesery delays
        auto currentTimeOffsetFromDateStart = getCurrentTimeNs() % NS_IN_DAY;
        for(; currentTimeOffsetFromDateStart < actionTimeOffsetFromDateStart
            ; currentTimeOffsetFromDateStart = getCurrentTimeNs() % NS_IN_DAY);
        act();
        m_exchangeOpen = !m_exchangeOpen; // Flip the state of the exchange open<->close
    }
}

void OrderManagement::rejectOrder(OrderRequest && request, const std::string rejectReason)
{
    std::cerr << "Order " << request.orderId << " was rejected:" << rejectReason << std::endl;
}

void OrderManagement::addRequestToQueue(OrderRequest && request)
{
    std::lock_guard<std::mutex> lock(m_ordersQueueMutex);
    m_ordersQueue.push(OrderInfo{request, false, getCurrentTimeNs()});
    m_queuedOrdersMap.emplace(request.orderId, &m_ordersQueue.back());
}

void OrderManagement::transmitRemoteRequests()
{
    using namespace std::chrono_literals; 
    std::queue<uint64_t> transmitTimes;
    while(!m_terminate) {
        uint64_t currentTime = getCurrentTimeNs();
        if (!m_exchangeOpen) {
            // reject all orders in the queue if exchange has been closed
            // while orders were waiting in the queue
            {
                std::lock_guard<std::mutex> locker(m_ordersQueueMutex);
                rejectOrdersInQueue("Exchange got closed while order was in the queue");
            }
            const auto currentTimeOffsetFromDateStart = currentTime % NS_IN_DAY;
            if (currentTimeOffsetFromDateStart >= m_config.closeTimeOffsetFromDayStartNs
                || (currentTimeOffsetFromDateStart < m_config.closeTimeOffsetFromDayStartNs
                && m_config.closeTimeOffsetFromDayStartNs - currentTimeOffsetFromDateStart < REGULAR_SLEEP_TIME_NS * 3)) {
                std::this_thread::sleep_for(std::chrono::nanoseconds(REGULAR_SLEEP_TIME_NS));
            } else {
                // if the time is close ot exchange open time sleep short period
                std::this_thread::sleep_for(std::chrono::nanoseconds(SHORT_SLEEP_TIME_NS));
            }
            continue;
        }
        // The exchange is open

        // remove all the items from the queue that are older then currentTime - window period 
        while (!transmitTimes.empty() 
            && transmitTimes.front() + m_config.windowSizeSec * NS_IN_SECOND < currentTime) {
            transmitTimes.pop();
        }
        // if there are less items in the transmitTimes queue then threshold
        // then try send the order otherwise sleep short period and check again
        if (transmitTimes.size() <= m_config.throttlingRate) {
            // Transmit the order if the queue is not empty
            bool transmitted = transmitOneOrder(currentTime);
            if (transmitted) {
                transmitTimes.push(currentTime);
            }
        } else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(SHORT_SLEEP_TIME_NS));
        }
    }
}

void OrderManagement::rejectOrdersInQueue(const std::string& rejectReason)
{
    while(!m_ordersQueue.empty() ) {
        auto nextOrder = m_ordersQueue.front();                
        if (!nextOrder.canceledFlag) {
            rejectOrder(std::move(nextOrder.request), 
                        rejectReason);
        }
        m_ordersQueue.pop();
    }
}

bool OrderManagement::transmitOneOrder(uint64_t& sendTime) {
    bool shouldSend = false;
    OrderInfo info;
    {
        std::lock_guard<std::mutex> locker(m_ordersQueueMutex);
        if (!m_ordersQueue.empty()) {
            info = std::move(m_ordersQueue.front());
            if (!info.canceledFlag) {
                shouldSend = true;
            }
            m_queuedOrdersMap.erase(info.request.orderId);
            m_ordersQueue.pop();
        }
    }
    if (shouldSend) {
        send(info.request);
        std::lock_guard<std::mutex> locker2(m_ordersStatsMutex);
        sendTime = getCurrentTimeNs();
        m_ordersStatsMap.emplace(info.request.orderId, 
                OrderStats{info.orderManagerReceiveTimeNs, sendTime, 0});
    }
    return shouldSend;
}

}  // ordermangement namespace