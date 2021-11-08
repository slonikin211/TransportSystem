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
        BusInfo info = t_system.GetBusInfoByBus(t_system.FindRouteByBusName(bus_name));

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
