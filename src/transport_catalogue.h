#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <algorithm>
#include <deque>

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
    std::unordered_set<const Stop*> route;
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


    const Bus* FindRouteByBusName(std::string_view name) const;
    const Stop* FindStopByName(std::string_view name) const;

private:
    std::unordered_set<Bus*> all_busses_ptrs_;
    std::deque<Bus> all_busses_;

    std::unordered_set<Stop*> all_stops_ptrs_;
    std::deque<Stop> all_stops_;
};
