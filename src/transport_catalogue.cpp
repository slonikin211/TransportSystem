#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <locale>

// Transport System

void TransportSystem::AddRoute(const Bus& bus)
{
    all_busses_.push_back(std::move(bus));
    Bus* bus_ptr = &all_busses_.back();
    all_busses_ptrs_.insert(bus_ptr);
}

void TransportSystem::AddStop(const Stop& stop)
{
    all_stops_.push_back(std::move(stop));
    Stop* stop_ptr = &all_stops_.back();
    all_stops_ptrs_.insert(stop_ptr);
}



const Bus* TransportSystem::FindRouteByBusName(std::string_view name) const
{
    auto to_find = std::find_if(all_busses_.begin(), all_busses_.end(),
        [&](const Bus& bus){
            return bus.name == name;
        });
    
    // This is so bad to return NULL, need to change this (but how?)
    return (to_find != all_busses_.end()) ? (&(*to_find)) : (nullptr);
}
    
const Stop* TransportSystem::FindStopByName(std::string_view name) const
{
    auto to_find = std::find_if(all_stops_.begin(), all_stops_.end(),
        [&](const Stop& bus){
            return bus.name == name;
        });
    
    // This is so bad to return NULL, need to change this
    return (to_find != all_stops_.end()) ? (&(*to_find)) : (nullptr);
}


BusInfo TransportSystem::GetBusInfoByBus(const Bus* bus) const
{   
    if (bus == nullptr)     // not found
    {
        return {};
    }
    // Find amount of stops
    size_t amount_of_stops = (!bus->cyclic_route) ? (bus->route.size() * 2u - 1u)
        : (bus->route.size() + (((*bus).route.front() == (*bus).route.back()) ? (0u) : (1u)));  // Need to check: s1 > s2 == s1 > s2 > s1

    // Count unique stops
    size_t amount_of_unique_stops;

    std::unordered_set<const Stop*> unique_stops, not_unique_stops;
    for (const auto stop: bus->route)
    {
        if (!unique_stops.count(stop) && !not_unique_stops.count(stop))
        {
            unique_stops.insert(stop);
        }
        else if (not_unique_stops.count(stop))
        {
            not_unique_stops.insert(stop);
        }
    }
    amount_of_unique_stops = unique_stops.size();
    
    // Compute route length
    double route_length = 0.0;
    Coordinates previous = {0.0, 0.0};
    bool first_stop = true;
    for (auto it = bus->route.begin(); it  != bus->route.end(); ++it)
    {
        if (!first_stop)    // avoiding first stop
        {
            route_length += ComputeDistance(previous, (*it)->coordinates);
        }
        previous = (*it)->coordinates;
        first_stop = false;
    }

    // Just go back if format is Bus X: A1 - A2 - ... - AN
    if (!bus->cyclic_route)
    {
        std::deque<const Stop*> route_back = {bus->route.begin(), bus->route.end()};
        first_stop = true;
        previous = {0.0, 0.0};
        for (auto it = route_back.rbegin(); it != route_back.rend(); ++it)
        {
            if (!first_stop)    // avoiding first stop
            {
                route_length += ComputeDistance(previous, (*it)->coordinates);
            }
            previous = (*it)->coordinates;
            first_stop = false;
        }
    }
    else
    {
        // To the first (Format Bus X: A1 > A2 > ... AN => AN > A1)
        // If AM > AM where M = [1, N] => ComputeDistance returns 0.0
        route_length += ComputeDistance(previous, (*(bus->route.begin()))->coordinates);
    }

    // Return info
    return {amount_of_stops, amount_of_unique_stops, route_length};
}


// Help functionality (TODO: make another file for this)

// trim from start (in place)
void ltrim(std::string_view &s) {
    while (s.front() == ' ') { s.remove_prefix(1u); }
}

// trim from end (in place)
void rtrim(std::string_view &s) {
    while (s.back() == ' ') { s.remove_suffix(1u); }
}

// trim from both ends (in place)
void trim(std::string_view &s) {
    ltrim(s);
    rtrim(s);
}