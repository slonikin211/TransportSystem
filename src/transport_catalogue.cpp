#include "transport_catalogue.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <locale>

// Transport System
using namespace transport_system;
using namespace transport_system::detail;

using namespace subjects::geo;
using namespace subjects::obj;
using namespace subjects::info;

#include <iostream>

void TransportSystem::AddRoute(const Bus& bus)
{
    // Init bus
    all_busses_.push_back(std::move(bus));
    Bus* bus_ptr = &all_busses_.back();

    // Link bus to stops
    for (auto stop: bus_ptr->route)
    {
        all_stops_info_[stop].buses.insert(bus_ptr);
        all_stops_info_[stop].stop = stop;
    }

    // Add to const bus ptrs
    p_all_busses_.push_back(bus_ptr);
}

void TransportSystem::AddStop(const Stop& stop)
{
    all_stops_.push_back(std::move(stop));

    const Stop* pstop = &all_stops_.back();
    all_stops_info_[pstop].stop = pstop;

    // Add to all stops ptrs
    p_all_stops_.push_back(pstop);
}

void TransportSystem::AddLinkStops(const std::pair<const Stop*, const Stop*>& connection, const double route)
{
    // Add link
    if (all_stops_connections_.count(connection) == 0u)     // first time to add
    {
        all_stops_connections_[connection] = route;
    }
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

double TransportSystem::FindConnectionValueByStops(const Stop* stop1, const Stop* stop2) const
{
    std::pair<const Stop*, const Stop*> key(stop1, stop2);
    if (all_stops_connections_.count(key) != 0u)
    {
        return all_stops_connections_.at(key);
    }
    else
    {
        std::pair<const Stop*, const Stop*> key2(stop2, stop1);
        if (all_stops_connections_.count(key2) != 0u)
        {
            return all_stops_connections_.at(key2);
        }
    }
    return 0.0;    // not found
}


double TransportSystem::ComputeGeoRoute(const Bus* bus) const
{
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
    return route_length;
}

double TransportSystem::ComputeRealRoute(const Bus* bus) const
{
    double route_length = 0;
    const Stop* previous = nullptr;
    bool first_stop = true;
    for (auto it = bus->route.begin(); it  != bus->route.end(); ++it)
    {
        if (!first_stop)    // avoiding first stop
        {
            const double route_val = FindConnectionValueByStops(previous, *it);
            route_length += route_val;
        }
        previous = *it;
        first_stop = false;
    }

    // Just go back if format is Bus X: A1 - A2 - ... - AN
    if (!bus->cyclic_route)
    {
        std::deque<const Stop*> route_back = {bus->route.begin(), bus->route.end()};
        first_stop = true;
        previous = nullptr;
        for (auto it = route_back.rbegin(); it != route_back.rend(); ++it)
        {
            if (!first_stop)    // avoiding first stop
            {
                const double route_val = FindConnectionValueByStops(previous, *it);
                route_length += route_val;
            }
            previous = *it;
            first_stop = false;
        }
    }
    else
    {
        // To the first (Format Bus X: A1 > A2 > ... AN => AN > A1)
        // If AM > AM where M = [1, N] => 0 or Connection(AM, AM)
        const Stop* first_stop = *bus->route.begin();
        const double route_val = FindConnectionValueByStops(previous, first_stop);
        route_length += route_val;
    }
    return route_length;
}

BusInfo TransportSystem::GetBusInfoByBus(const Bus* bus) const
{   
    if (bus == nullptr)     // not found
    {
        return {nullptr, 0u, 0u, 0.0, 0.0};
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
    
    double geo_route_length = ComputeGeoRoute(bus);     
    double real_route_length = ComputeRealRoute(bus);
    
    // Return info
    return {bus, amount_of_stops, amount_of_unique_stops, real_route_length, real_route_length / geo_route_length};
}

StopInfo TransportSystem::GetStopInfoByStop(const Stop* stop) const
{
    if (stop == nullptr || !all_stops_info_.count(stop))
    {
        return {nullptr, {}};
    }
    return all_stops_info_.at(stop);
}

const std::deque<const subjects::obj::Bus*>& TransportSystem::GetBusesPointers() const
{
    return p_all_busses_;
}

const std::deque<const subjects::obj::Stop*>& TransportSystem::GetStopPointers() const
{
    return p_all_stops_;
}

const std::deque<StopInfo>& TransportSystem::GetStopInfo() const
{
    static std::deque<StopInfo> stop_infos;
    if (stop_infos.empty()) {
        for (const auto& el: all_stops_info_) {
            stop_infos.push_back(el.second);
        }
    }
    return stop_infos;
}

// Help functionality (TODO: make another file for this)

// trim from start (in place)
void help::ltrim(std::string_view &s) {
    while (s.front() == ' ') { s.remove_prefix(1u); }
}

// trim from end (in place)
void help::rtrim(std::string_view &s) {
    while (s.back() == ' ') { s.remove_suffix(1u); }
}

// trim from both ends (in place)
void help::trim(std::string_view &s) {
    ltrim(s);
    rtrim(s);
}