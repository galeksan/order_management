#include "MockOrdersGenerator.h"
#include "OrderManagement.h"
#include <random>
#include <iostream>

namespace ordermanagement {

MockOrdersGenerator::MockOrdersGenerator(OrderManagement* orderManager, uint8_t prefix) 
    : m_orderManager(orderManager)
    , m_clientPrefix(prefix)
    , m_terminate(false) 
    , m_orderSeqNum(0)
{
    m_generatorThread = std::make_unique<std::thread>(&MockOrdersGenerator::generateOrders, this);
}

MockOrdersGenerator::~MockOrdersGenerator()
{
    m_terminate = true;
    m_generatorThread->join();
}

void MockOrdersGenerator::generateOrders()
{
    while(!m_terminate) {
        OrderRequest nextRequest;
        auto nextId = getNextSeqNumber();
        if (nextId % 10 == 1) {
            // Every 10th order will be canceled
            nextRequest.orderId = nextId - 1;
            std::cout << "sending cancel for " << nextRequest.orderId << std::endl;
            m_orderManager->onData(std::move(nextRequest), RequestType::Cancel);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        if (nextId % 10 == 6) {
            // Every 10th order will be canceled
            nextRequest.orderId = nextId - 1;
            std::cout << "sending modify for " << nextRequest.orderId << std::endl;
            m_orderManager->onData(std::move(nextRequest), RequestType::Modify);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        nextRequest.orderId = nextId;
        std::cout << "sending " << nextRequest.orderId << std::endl;
        m_orderManager->onData(std::move(nextRequest), RequestType::New);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

uint64_t MockOrdersGenerator::getNextSeqNumber()
{
    uint64_t seqNum = m_orderSeqNum++;
    uint64_t clientPrefix = m_clientPrefix;
    uint64_t tmp = seqNum;
    while (tmp) {
        clientPrefix *= 10;
        tmp /= 10;
    }
    return seqNum + clientPrefix;
}

}  // ordermangement namespace
