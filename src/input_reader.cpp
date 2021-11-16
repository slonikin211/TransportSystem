#include "input_reader.h"
#include "transport_catalogue.h"

#include <map>


using namespace transport_system::detail;
using namespace help;

namespace transport_system::init::detail 
{
    Stop GetInitStopFromQuery(std::string_view str)
    {
        // Format
        // Stop X: coordX, coordY
        
        // Stop name
        size_t start_position = 5u; // Stop => 4 letters
        std::string_view stop_name = str.substr(start_position, str.find(":") - start_position);


        // Coord Latitude
        start_position += stop_name.size() + 1u;    // includes ":"
        std::string_view coordLat = str.substr(start_position, str.find(",", start_position) - start_position);

        // If default stop query (exludes additional linked stop information)
        std::string_view coordLng;
        start_position += coordLat.size() + 1u;     // includes ","
        if (str.find(",", start_position) == str.npos)
        {
            // Coord Longtitude without additional stops
            coordLng = str.substr(start_position, str.size() - start_position);
        }
        else
        {
            coordLng = str.substr(start_position, str.find(",", start_position) - start_position);
        }


        // Remove spaces
        trim(stop_name);
        trim(coordLat);
        trim(coordLng);

        Stop stop = {
            std::string(stop_name), 
            {std::stod(std::string(coordLat)) , std::stod(std::string(coordLng))} 
        };

        return stop;
    }

    std::deque<std::string> ParseStopsFromQuery(std::string_view str)
    {
        using namespace std::string_view_literals;
        
        // Format: Stop stop: coord1, coord2, d1m stop1, d2m stop2.....
        // => skip ':' then ',' and ',' if exist
        size_t start_position = str.find(':'); 
        str.remove_prefix(start_position + 1u);  // now " coord1, coord2..."
        
        start_position = str.find(',');
        str.remove_prefix(start_position + 1u);  // now " coord2..."

        start_position = str.find(',');
        if (start_position == str.npos)     // if here is no stops
        {
            return {};
        }
        str.remove_prefix(start_position + 1u);  // now " stop1 , stop2 and etc."
        
        std::deque<std::string> result;
        char delimiter = ',';

        while (str != ""sv)    // while str is not empty
        {
            std::string_view current_stop;
            const auto to_position = str.find(delimiter);

            current_stop = (to_position != std::string_view::npos) ? (str.substr(0, to_position)) : (str);

            // for next iteration
            str.remove_prefix(current_stop.size());    // ", " includes    
            if (str.front() == delimiter)
            {
                str.remove_prefix(1u);  // delete delimeter
            }

            // save data
            trim(current_stop);
            result.push_back(std::string(current_stop));
        }
        return result;
    }

    bool CheckCycleRoute(std::string_view str)
    {   
        // if true => delimiter is '>' => cyclic route
        return (str.find(">") != std::string_view::npos);
    }

    std::deque<std::string> ParseRouteFromQuery(std::string_view str)
    {
        using namespace std::string_view_literals;
        // Fromat 1:
        // stop1 - stop2 - ... stopN
        // Format 2:
        // stop1 > stop2 > ... stopN > stop1
        // Hint:
        // Format1 => stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1

        std::deque<std::string> result;

        // Define type of route (> or -)
        char delimiter = '>';
        if (!CheckCycleRoute(str))
        {
            delimiter = '-';
        }

        while (str != ""sv)    // while str is not empty
        {
            std::string_view current_stop;
            const auto to_position = str.find(delimiter);

            current_stop = (to_position != std::string_view::npos) ? (str.substr(0, to_position)) : (str);

            // for next iteration
            str.remove_prefix(current_stop.size());    // "> " includes    
            if (str.front() == delimiter)
            {
                str.remove_prefix(1u);  // delete delimeter
            }

            // save data
            trim(current_stop);
            result.push_back(std::string(current_stop));
        }

        return result;
    }

    Bus InitBusFromQuery(TransportSystem& t_system, std::string_view str)
    {
        // Format 1:
        // Bus X: stop1 - stop2 - ... stopN
        // Format 2:
        // stop1 > stop2 > ... > stopN > stop1
        // Hint:
        // Format1 => stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1
        
        // Bus name
        size_t start_position = 4u; // Bus => 3 letters
        std::string_view bus_name = str.substr(start_position, str.find(":") - start_position);


        // Route
        start_position += bus_name.size() + 1u;     // includes ":"
        std::string_view route = str.substr(start_position, str.size() - start_position);

        // Parse route string into deque
        std::deque<std::string> route_set = ParseRouteFromQuery(route);
        trim(bus_name);

        // Convert string set to Stops*
        std::deque<const Stop*> stops_of_route;
        for (const auto& stop_str: route_set)
        {
            const Stop* stop = t_system.FindStopByName(stop_str);
            if (stop != nullptr) 
            {
                stops_of_route.push_back(stop);
            }
        }
        
        // Init bus object
        Bus bus = { 
            std::string(bus_name),
            stops_of_route,
            CheckCycleRoute(route)
        };
        return bus;
    }

}

namespace transport_system::init::query
{
    struct InitQueryBus
    {
        std::string bus_name;
        std::deque<std::string> stops;
        bool isCycle;
    };

    InitQueryBus ParseQueryBusFromStr(std::string_view str)
    {
        using namespace transport_system::init::detail;
        // Format 1:
        // Bus X: stop1 - stop2 - ... stopN
        // Format 2:
        // stop1 > stop2 > ... > stopN > stop1
        // Hint:
        // Format1 => stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1
        
        // Bus name
        size_t start_position = 4u; // Bus => 3 letters
        std::string_view bus_name = str.substr(start_position, str.find(":") - start_position);

        // Route
        start_position += bus_name.size() + 1u;     // includes ":"
        std::string_view route = str.substr(start_position, str.size() - start_position);

        // Parse route string into deque
        std::deque<std::string> route_deque = ParseRouteFromQuery(route);
        trim(bus_name);

        return { std::string(bus_name), route_deque, CheckCycleRoute(str) };
    }

    struct InitQueryStop
    {
        std::string stop_name;
        transport_system::detail::Coordinates coords;
        std::map<std::string, double> stops; // Format: {stop; route_length}
    };

    InitQueryStop ParseQueryStopFromStr(std::string_view str)
    {
        using namespace transport_system::init::detail;
        Stop stop = GetInitStopFromQuery(str);
        std::deque<std::string> stops =  ParseStopsFromQuery(str);
        
        std::map<std::string, double> stops_with_route_length;

        for (const auto& to_stop: stops)
        {
            std::string_view stop_sv = to_stop;
            size_t space_position = stop_sv.find(' ');
            std::string_view route_length = stop_sv.substr(0, space_position);
            std::string_view stop_name = stop_sv.substr(space_position + 4u, stop_sv.size() - route_length.size() - 1u);
            route_length.remove_suffix(1u);     // remove 'm'
            trim(stop_name);
            stops_with_route_length[std::string(stop_name)] = std::stod(std::string(route_length));
        }

        return { stop.name, stop.coordinates, stops_with_route_length };
    }
}

namespace transport_system::init::detail
{
    void SetStopsConnectionsFromQuery(TransportSystem& system, const transport_system::init::query::InitQueryStop& query)
    {
        // Format
        // Stop X: coordLat, coordLng, D1m to  stop1, D2m to stop2 and etc.
        const Stop* stop1 = system.FindStopByName(query.stop_name);

        // Link
        for (const auto& current_stop: query.stops)
        {
            const Stop* stop2 = system.FindStopByName(current_stop.first); // find by name
            if (stop2 == nullptr)
            {
                continue;
            }
            double route_length = current_stop.second;  // route length

            // Connect!
            std::pair<const Stop*, const Stop*> key_to_add(stop1, stop2);
            system.AddLinkStops(key_to_add, route_length);
        }
    }

    std::deque<const Stop*> GetAllStopPointersFromInitQueryBus(TransportSystem& t_system, const transport_system::init::query::InitQueryBus& query)
    {
        std::deque<const Stop*> route;
        for (const auto& stop: query.stops)
        {
            const Stop* stop_ptr = t_system.FindStopByName(stop);
            if (stop_ptr != nullptr)
            {
                route.push_back(stop_ptr);
            }
        }
        return route;
    }

    void ProcessInitQueries(TransportSystem& t_system, const std::deque<transport_system::init::query::InitQueryStop>& stop_queries, 
            const std::deque<transport_system::init::query::InitQueryBus>& bus_queries)
    {
        // // First of all we need to init stops, then routes (buses)

        // Process init Stop queries
        for (const auto& query: stop_queries)
        {
            t_system.AddStop({query.stop_name, query.coords});
        }

        // Process Link Stops queries
        for (const auto& query: stop_queries)
        {
            SetStopsConnectionsFromQuery(t_system, query);
        }

        // Process Bus queries
        for (const auto& query: bus_queries)
        {
            Bus bus{query.bus_name, GetAllStopPointersFromInitQueryBus(t_system, query), query.isCycle};
            t_system.AddRoute(bus);
        }
    }
}

void transport_system::init::InputReadFromCin(TransportSystem& t_system)
{
    using namespace transport_system::init::query;
    size_t n;
    std::cin >> n;

    std::deque<InitQueryBus> bus_queries;
    std::deque<InitQueryStop> stop_queries;
    for (size_t i = 0; i <= n; ++i)
    {
        std::string str;
        std::getline(std::cin, str);
        if (!str.empty())
        {
            if (str.at(0) == 'B')
            {
                bus_queries.push_back(ParseQueryBusFromStr(str));
            }
            else if (str.at(0) == 'S')
            {
                stop_queries.push_back(ParseQueryStopFromStr(str));
            }
        }
    }

    transport_system::init::detail::ProcessInitQueries(t_system, stop_queries, bus_queries);
}
