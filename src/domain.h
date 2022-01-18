#pragma once

#include <string>
#include <deque>
#include <set>

#include "geo.h"

namespace obj
{
    // Stop structure include stop name and coordinates
    struct Stop
    {
        std::string name;
        geo::Coordinates coordinates;
    };

    // Bus structure include bus name and route
    struct Bus
    {
        std::string name;
        std::deque<const Stop*> route;
        bool cyclic_route;
    };
} // obj 

namespace info
{
    // Bus info for output
    struct BusInfo
    {
        const obj::Bus* bus;
        size_t amount_of_stops;
        size_t amount_of_unique_stops;
        double route_length;
        double curvature;
    };


    // Stop info for output
    struct StopInfo
    {
        const obj::Stop* stop;
        std::set<const obj::Bus*> buses;
    };
}   