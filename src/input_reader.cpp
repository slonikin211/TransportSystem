#include "input_reader.h"
#include "transport_catalogue.h"

Stop GetInitStopFromQuery(std::string_view str)
{
    // Format
    // Stop X: coordX, coordY
    
    // Stop name
    size_t start_position = 5u; // Stop => 4 letters
    std::string_view stop_name = str.substr(start_position, str.find(":") - start_position);

    // Coord Latitude
    start_position += stop_name.size() + 2u;    // includes ": "
    std::string_view coordLat = str.substr(start_position, str.find(",", start_position) - start_position);

    // Coord Longtitude
    start_position += coordLat.size() + 2u;     // includes ", "
    std::string_view coordLng = str.substr(start_position, str.size() - start_position);

    Stop stop = {
        std::string(stop_name), 
        {std::stod(std::string(coordLat)) , std::stod(std::string(coordLng))} 
    };

    return stop;
}

std::unordered_set<std::string> ParseRouteFromQuery(std::string_view str)
{
    // Fromat 1:
    // stop1 - stop2 - ... stopN
    // Format 2:
    // stop1 > stop2 > ... stopN > stop1
    // Hint:
    // Format1 => stop1 > stop2 > ... > stopN-1 > stopN > stopN-1 > ... > stop2 > stop1

    std::unordered_set<std::string> result;

    // Define type of route (> or -)
    char delimiter = '>';
    if (str.find(">") == std::string_view::npos)
    {
        delimiter = '-';
    }

    // Lambda for remove spaces from beginning and end of string_view from queries
    auto simple_space_remover = [](std::string_view& sv) {
        if (sv.front() == ' ') { sv.remove_prefix(1u); }
        if (sv.back() == ' ') { sv.remove_suffix(1u); }
    };

    while (str.size() != std::string::npos - 1u)    // while string is not empty
    {
        std::string_view current_stop;
        const auto to_position = str.find(delimiter);

        current_stop = (to_position != std::string_view::npos) ? (str.substr(0, to_position)) : (str);

        // for next iteration
        str.remove_prefix(current_stop.size() + 2u);    // "> " includes    

        // save data
        simple_space_remover(current_stop);
        result.insert(std::string(current_stop));
    }
    return result;
}

Bus GetInitBusFromQuery(TransportSystem& t_system, std::string_view str)
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
    start_position += bus_name.size() + 2u;     // includes ": "
    std::string_view route = str.substr(start_position, str.size() - start_position);

    // Parse route string into set
    std::unordered_set<std::string> route_set = ParseRouteFromQuery(route);

    // Convert string set to Stops*
    std::unordered_set<const Stop*> stops_of_route;
    for (const auto& stop_str: route_set)
    {
        const Stop* stop = t_system.FindStopByName(stop_str);
        if (stop != nullptr) 
        {
            stops_of_route.insert(stop);
        }
    }
    
    // Init bus object
    Bus bus = { std::string(bus_name), stops_of_route };

    return bus;
}

void ProcessInitQueries(TransportSystem& t_system, const std::unordered_set<std::string>& queries)
{
    // First of all we need to init stops, then routes (buses)
    std::unordered_set<std::string_view> stop_queries, bus_queries;

    for (const auto& str : queries)
    {
        if (str.at(0) == 'B')   // Bus
        {
            bus_queries.insert(str);
        }
        else if (str.at(0) == 'S')  // Stop
        {
            stop_queries.insert(str);
        }
    }

    // Process Stop queries
    for (const auto& query: stop_queries)
    {
        Stop stop = GetInitStopFromQuery(query);
        t_system.AddStop(stop);
    }

    // Process Bus queries
    for (const auto& query: bus_queries)
    {
        Bus bus = GetInitBusFromQuery(t_system, query);
        t_system.AddRoute(bus);
    }
}

void InputRead(TransportSystem& t_system)
{
    std::unordered_set<std::string> init_queries;

    size_t n;
    std::cin >> n;

    for (size_t i = 0; i < n; ++i)
    {
        std::string str;
        std::getline(std::cin, str);
        init_queries.insert(str);
    }

    ProcessInitQueries(t_system, init_queries);
}
