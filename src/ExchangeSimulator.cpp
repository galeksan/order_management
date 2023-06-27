#include "ExchangeSimulator.h"
#include <iostream>
#include <algorithm>
#include <chrono>

namespace ordermanagement {

ExchangeResponseSimulator::ExchangeResponseSimulator(OrderManagement* manager) 
    : m_manager(manager)
    , m_rd()
    , m_gen(m_rd())
    , m_distr(0, static_cast<int>(ResponseType::Reject) + 1)
{
    m_respondThread = std::make_unique<std::thread>(&ExchangeResponseSimulator::respond, this);
}

ExchangeResponseSimulator::~ExchangeResponseSimulator()
{
    m_terminated = true;
    m_respondThread->join();
}

void ExchangeResponseSimulator::sendLogon(const Logon& logon) {
    m_loggedIn = true;
}
    
void ExchangeResponseSimulator::sendLogout(const Logout& logon) {
    m_loggedIn = false;
}

void ExchangeResponseSimulator::send(const OrderRequest& request) {
    std::unique_lock<std::mutex> locker(m_requestsLock);
    m_requests.push(request.orderId);
    std::cout << "Exchange got " << request.orderId << std::endl;
}

void ExchangeResponseSimulator::respond() {
    while(!m_terminated) {
        {
            std::unique_lock<std::mutex> locker(m_requestsLock);
            if (!m_requests.empty()) {
                uint64_t orderId = m_requests.front();
                m_requests.pop();
                ResponseType responseType = static_cast<ResponseType>(m_distr(m_gen));
                OrderResponse response{orderId, responseType};
                m_manager->onData(std::move(response));
            }
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

} // ordermangement namespace
