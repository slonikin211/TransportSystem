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

void SetStopsConnectionsFromQuery(TransportSystem& system, std::string_view str)
{
    // Format
    // Stop X: coordLat, coordLng, D1m to  stop1, D2m to stop2 and etc.
    
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
        return;
    }

    coordLng = str.substr(start_position, str.find(",", start_position) - start_position);
    start_position += coordLng.size() + 1u;     // includes ","

    // Get all stops info and link it
    trim(stop_name);
    const Stop* stop1 = system.FindStopByName(stop_name);

    // Get all stops info
    std::string_view additional_stops = str.substr(start_position, str.size() - start_position);
    trim(additional_stops);
    std::deque<std::string> stop_to_link_info = ParseStopsFromQuery(additional_stops);

    // Link
    for (const auto& current_stop: stop_to_link_info)
    {
        // Prepare data
        size_t current_position = 0u;

        // Route length
        std::string_view current_stop_route_length = current_stop.substr(current_position, current_stop.find(" "));
        current_stop_route_length.remove_suffix(1u);    // remove suffix 'm'

        // Stop name
        current_position += current_stop_route_length.size() + 4u; // "220m to Stop" => "m to " skip
        // using string because of incorrect working of saving data to string_view (stop_name with spaces incorrect)
        std::string current_stop_name_s = std::string(current_stop.substr(current_position, current_stop.size() - current_position));
        std::string_view current_stop_name = current_stop_name_s;

        // Final prepare
        trim(current_stop_name);
        const Stop* stop2 = system.FindStopByName(current_stop_name);
        if (stop2 == nullptr)
        {
            continue;
        }


        double route_length = std::stod(std::string(current_stop_route_length));

        // Connect!
        Connection to_add = 
        {
            stop1, 
            stop2,
            route_length
        };

        system.AddLinkStops(to_add);
    }
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
    if (str.find(">") == std::string_view::npos)
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

void ProcessInitQueries(TransportSystem& t_system, const std::deque<std::string>& queries)
{
    // First of all we need to init stops, then routes (buses)
    std::deque<std::string_view> stop_queries, bus_queries;

    for (const auto& str : queries)
    {
        if (str.at(0) == 'B')   // Bus
        {
            bus_queries.push_back(str);
        }
        else if (str.at(0) == 'S')  // Stop
        {
            stop_queries.push_back(str);
        }
    }

    // Process init Stop queries
    for (const auto& query: stop_queries)
    {
        Stop stop = GetInitStopFromQuery(query);
        t_system.AddStop(stop);
    }

    // Process Link Stops queries
    for (const auto& query: stop_queries)
    {
        SetStopsConnectionsFromQuery(t_system, query);
    }

    // Process Bus queries
    for (const auto& query: bus_queries)
    {
        Bus bus = InitBusFromQuery(t_system, query);
        t_system.AddRoute(bus);
    }
}

void InputRead(TransportSystem& t_system)
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

    ProcessInitQueries(t_system, init_queries);
}
