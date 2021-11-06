#pragma once

#include <iostream>
#include <unordered_set>
#include <string>

#include "transport_catalogue.h"

// Parse string into Stop
Stop GetInitStopFromQuery(std::string_view str);

// Parse route into std::unordered_set<Stop*> 
std::unordered_set<std::string> ParseRouteFromQuery(std::string_view str);

// Parse string into Bus
Bus GetInitBusFromQuery(TransportSystem& t_system, std::string_view str);

// Process queries from container (unordered set) and init TransportSystem with them
void ProcessInitQueries(TransportSystem& t_system, const std::unordered_set<std::string>& queries);

// Just convert cin to queries
void InputRead(TransportSystem& t_system);