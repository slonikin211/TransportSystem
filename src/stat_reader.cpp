#include "stat_reader.h"
#include "geo.h"

#include <iostream>
#include <unordered_set>
#include <string_view>
#include <string>
#include <deque>
#include <sstream>

std::string GetNameFromBusInfoQuery(std::string_view str)
{
    // Bus => 3 letters + space => start position = 4
    std::string_view to_return = str.substr(4u, str.find(":") - 4u);
    trim(to_return);
    return std::string(to_return);
}

BusInfo GetBusInfo(TransportSystem& t_system, std::string_view name)
{
    const Bus* bus = t_system.FindRouteByBusName(name);
    if (bus == nullptr)     // not found
    {
        return {};
    }
    
    // Find amount of stops
    size_t amount_of_stops = (!bus->cyclic_route) ? (bus->route.size() * 2u - 1u)
        : (bus->route.size() + (((*bus).route.front() == (*bus).route.back()) ? (0u) : (1u)));  // Need to check: s1 > s2 == s1 > s2 > s1

    // Count unique stops
    size_t amount_of_unique_stops;

    if (!bus->cyclic_route)
    {
        amount_of_unique_stops = bus->route.size();
    }
    else
    {
        std::unordered_set<const Stop*> unique_stops, not_unique_stops;
        for (const auto stop: bus->route)
        {
            if (!unique_stops.count(stop) && !not_unique_stops.count(stop))
            {
                unique_stops.insert(stop);
            }
            else if (unique_stops.count(stop))
            {
                unique_stops.erase(stop);
                not_unique_stops.insert(stop);
            }
        }
        amount_of_unique_stops = unique_stops.size() + (((*bus).route.front() == (*bus).route.back()) ? (1u) : (0u));
    }

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
        // To the first
        route_length += ComputeDistance(previous, (*(bus->route.begin()))->coordinates);
    }

    // Return info
    return {amount_of_stops, amount_of_unique_stops, route_length};
}

std::string ProcessDBQueries(TransportSystem& t_system, const std::deque<std::string>& queries)
{
    using namespace std::string_literals;
    std::deque<std::string_view> bus_info_queries;

    for (const auto& str: queries)
    {
        if (str.at(0) == 'B')   // Bus
        {
            bus_info_queries.push_back(str);
        }
    }

    // Process Bus Info queries
    std::stringstream ss;
    for (const auto& query: bus_info_queries)
    {
        std::string bus_name = GetNameFromBusInfoQuery(query);
        BusInfo info = GetBusInfo(t_system, bus_name);

        if (info.amount_of_stops != 0u)
        {
            ss << "Bus " << bus_name << ": " << 
                info.amount_of_stops << " stops on route, " <<
                info.amount_of_unique_stops << " unique stops, " <<
                info.route_length << " route length" << std::endl;
        }
        else
        {
            ss << "Bus " << bus_name << ": not found" << std::endl;
        }
    }
    return ss.str();
}

void InputReadDBQueries(TransportSystem& t_system)
{
    std::deque<std::string> init_queries;

    size_t n;
    std::cin >> n;

    for (size_t i = 0; i <= n; ++i)
    {
        std::string str;
        std::getline(std::cin, str);
        if (!str.empty())
        {
            init_queries.push_back(str);
        }
    }

    std::cout << ProcessDBQueries(t_system, init_queries) << std::endl;
}
