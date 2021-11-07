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
    // TODO: I don't remember why
    assert(bus != nullptr);

    BusInfo info;

    return info;
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