// Provides definitions for basic structs provided by the exercise

#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace ordermanagement {

constexpr uint64_t NS_IN_DAY = 1000000000ull * 24 * 3600;
constexpr uint64_t NS_IN_SECOND = 1000000000ull;

struct Logon {
    std::string username; std::string password;
};

struct Logout {
    std::string username; 
};

enum class RequestType {
    Unknown = 0, 
    New = 1, 
    Modify = 2, 
    Cancel = 3
};

struct OrderRequest {
    int symbolId;
    double price;
    uint64_t qty;
    char side; // possible values 'B' or 'S' uint64_t m_orderId;
    uint64_t orderId;
};

enum class ResponseType {
    Unknown = 0,
    Accept = 1,
    Reject = 2
};

struct OrderResponse {
    uint64_t orderId;
    ResponseType responseType; 
};

struct OrderInfo {
    OrderRequest request;
    bool canceledFlag;
    uint64_t orderManagerReceiveTimeNs;
};

struct OrderStats {
    uint64_t orderManagerReceiveTimeNs;
    uint64_t requestSendTimeNs;
    uint64_t responseReceivalTimeNs; 
};

std::ostream& operator<<(std::ostream& ofs, const OrderStats& stats);
std::ostream& operator<<(std::ostream& ofs, const OrderResponse& response);
std::uint64_t getCurrentTimeNs();

} // ordermanagement namespace

#endif