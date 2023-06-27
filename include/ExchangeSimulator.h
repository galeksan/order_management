// IExchangeSimulator/ExchangeResponseSimulator classes imulate exchange functionality.
// Exchange receives orders from OrderManagement class and call callbacks to report
// Dummy response for each order. IExchangeSimulator provides interface of an Exchange
// and ExchangeResponseSimulator implements a mock exchange for testing purposes
// Please note that I use dependency injection for this project components testing.

#ifndef EXCHANGE_SIMULATOR_H 
#define EXCHANGE_SIMULATOR_H

#include <mutex>
#include <queue>
#include <string>
#include <random>

#include "OrderManagement.h"

namespace ordermanagement {

class OrderRequest;

class IExchangeSimulator {
public:
    virtual ~IExchangeSimulator() = default;
    virtual void send(const OrderRequest& request) = 0;
    virtual void sendLogon(const Logon& logon) = 0;
    virtual void sendLogout(const Logout& logout) = 0;
}; 

// exchange response mock simulator will only permit one 
// client(OrderManagement), as it is only used for testing
class ExchangeResponseSimulator : public IExchangeSimulator {
public:
    ExchangeResponseSimulator(OrderManagement* manager);
    ~ExchangeResponseSimulator();
    void sendLogon(const Logon& logon) override;
    void sendLogout(const Logout& logout) override;
    void send(const OrderRequest& request) override;

private:
    void respond();
private:
    OrderManagement* m_manager;
    std::mutex m_requestsLock;
    std::queue<uint64_t> m_requests;
    std::atomic_bool m_loggedIn = false;
    std::atomic_bool m_terminated = false;
    std::unique_ptr<std::thread> m_respondThread;
    
    std::random_device m_rd;
    std::mt19937 m_gen;
    std::uniform_int_distribution<> m_distr;
};

} // ordermangement namespace

#endif