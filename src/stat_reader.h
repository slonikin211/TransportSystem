#pragma once

#include "transport_catalogue.h"

// Get stop name from StopInfo query
std::string GetNameFromStopInfoQuery(std::string_view str);

// Get bus name from BusInfo query
std::string GetNameFromBusInfoQuery(std::string_view str);

// Process queries from container (unordered set)
std::string ProcessDBQueries(TransportSystem& t_system, const std::deque<std::string>& queries);

// Just convert cin to queries
void InputReadDBQueries(TransportSystem& t_system);