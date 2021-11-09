#include "stat_reader.h"
#include "geo.h"

#include <iostream>
#include <unordered_set>
#include <string_view>
#include <string>
#include <deque>
#include <sstream>

std::string GetNameFromStopInfoQuery(std::string_view str)
{
    // Stop => 4 letters + space => start position = 5
    std::string_view to_return = str.substr(5u, str.find(":") - 5u);
    trim(to_return);
    return std::string(to_return);
}

std::string GetNameFromBusInfoQuery(std::string_view str)
{
    // Bus => 3 letters + space => start position = 4
    std::string_view to_return = str.substr(4u, str.find(":") - 4u);
    trim(to_return);
    return std::string(to_return);
}

std::string ProcessDBQueries(TransportSystem& t_system, const std::deque<std::string>& queries)
{
    using namespace std::string_literals;
    std::deque<std::string_view> all_queries;

    for (const auto& str: queries)
    {
        all_queries.push_back(str);
    }

    // Process queries
    std::stringstream ss;
    size_t i = 0u;
    for (const auto& query: all_queries)
    {
        if (query.at(0) == 'B')     // Bus
        {
            std::string bus_name = GetNameFromBusInfoQuery(query);
            BusInfo info = t_system.GetBusInfoByBus(t_system.FindRouteByBusName(bus_name));

            if (info.amount_of_stops != 0u)
            {
                ss << "Bus " << bus_name << ": " << 
                    info.amount_of_stops << " stops on route, " <<
                    info.amount_of_unique_stops << " unique stops, " <<
                    info.route_length << " route length, " << 
                    info.curvature << " curvature";
            }
            else
            {
                ss << "Bus " << bus_name << ": not found";
            }
        }
        else if (query.at(0) == 'S')    // Stop
        {
            std::string stop_name = GetNameFromStopInfoQuery(query);
            const Stop* stop = t_system.FindStopByName(stop_name);

            if (stop == nullptr)
            {
                ss << "Stop " << stop_name << ": not found";
            }
            else
            {
                StopInfo info = t_system.GetStopInfoByStop(stop);
                if (info.buses.empty())
                {
                    ss << "Stop " << stop_name << ": no buses";
                }
                else
                {
                    ss << "Stop " << stop_name << ": buses ";

                    // Sort by name
                    std::set<std::string_view> sorted;
                    for (const auto& bus: info.buses)
                    {
                        sorted.insert(bus->name);
                    }

                    // Out
                    for (const auto& bus: sorted)
                    {
                        ss << bus << " ";
                    }
                    ss.seekp(-1, ss.cur);    // remove space
                }
            }
        }
    
        if (i++ < all_queries.size() - 1u)
        {
            ss << std::endl;
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

    std::cout << ProcessDBQueries(t_system, init_queries);
}
