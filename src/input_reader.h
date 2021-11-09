#pragma once

#include <iostream>
#include <unordered_set>
#include <string>

#include "transport_catalogue.h"

// Parse string into Stop
Stop GetInitStopFromQuery(std::string_view str);

// Parse stops into std::deque<std::string> 
std::deque<std::string> ParseStopsFromQuery(std::string_view str);

// Parse string of stops into std::deque<Connection> and add it to TransportSystem
void SetStopsConnectionsFromQuery(TransportSystem& system, std::string_view str);

// Check for cyclic route
bool CheckCycleRoute(std::string_view str);

// Parse route into std::deque<std::string> 
std::deque<std::string> ParseRouteFromQuery(std::string_view str);

// Parse string into Bus
Bus InitBusFromQuery(TransportSystem& t_system, std::string_view str);

// Process queries from container (unordered set) and init TransportSystem with them
void ProcessInitQueries(TransportSystem& t_system, const std::deque<std::string>& queries);

// Just convert cin to queries
void InputRead(TransportSystem& t_system);