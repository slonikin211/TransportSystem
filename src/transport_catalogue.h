#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <set>

#include "geo.h"

// Stop structure include stop name and coordinates
struct Stop
{
    std::string name;
    Coordinates coordinates;
};

// Bus structure include bus name and route
struct Bus
{
    std::string name;
    std::deque<const Stop*> route;
    bool cyclic_route;
};


// Bus info for output
struct BusInfo
{
    size_t amount_of_stops;
    size_t amount_of_unique_stops;
    double route_length;
    double curvature;
};


// Stop info for output
struct StopInfo
{
    std::set<const Bus*> buses;
};


// Connection between 2 stops includes route length and curvatur between them
struct Connection
{
    const Stop* stop1;
    const Stop* stop2;
    double route_length;
};

// Hasher for connection
class ConnectionHasher
{
public:
    size_t operator()(Connection* conn) const
    {
        return (
            static_cast<size_t>(c_hasher_(&conn->stop1)) +
            static_cast<size_t>(c_hasher_(&conn->stop1)  * 37u)
        );
    }
private:
    std::hash<void*> c_hasher_;
};



// Transport system class which includes next functionality
// 1. Add route to DB
// 2. Add stop to DB
// 3. Find route by name
// 4. Find stop by name
// 5. Get information about route

class TransportSystem
{
public:
    void AddRoute(const Bus& bus);
    void AddStop(const Stop& stop);
    void AddLinkStops(const Connection& connection);

    const Bus* FindRouteByBusName(std::string_view name) const;
    const Stop* FindStopByName(std::string_view name) const;
    const Connection* FindConnectionByStops(const Stop* stop1, const Stop* stop2) const;

    BusInfo GetBusInfoByBus(const Bus* bus) const;
    StopInfo GetStopInfoByStop(const Stop* stop) const;

private:
    double ComputeGeoRoute(const Bus* bus) const;
    double ComputeRealRoute(const Bus* bus) const;

private:
    std::unordered_set<Bus*> all_busses_ptrs_;
    std::deque<Bus> all_busses_;

    std::unordered_set<Stop*> all_stops_ptrs_;
    std::deque<Stop> all_stops_;
    std::unordered_map<const Stop*, StopInfo> all_stops_info_;

    std::unordered_set<Connection*, ConnectionHasher> all_stops_connections_ptrs_;
    std::deque<Connection> all_stops_connections_;
};



// Help functionality (TODO: make another file for this)

// trim from start (in place)
void ltrim(std::string_view &s);

// trim from end (in place)
void rtrim(std::string_view &s);

// trim from both ends (in place)
void trim(std::string_view &s);