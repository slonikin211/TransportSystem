#pragma once

#include "transport_catalogue.h"

// Get bus name from BusInfo query
std::string GetNameFromBusInfoQuery(std::string_view str);

// Get bus info (route) from TransportSystem
BusInfo GetBusInfo(TransportSystem& t_system, std::string_view name);

// Process queries from container (unordered set)
std::string ProcessDBQueries(TransportSystem& t_system, const std::deque<std::string>& queries);

// Just convert cin to queries
void InputReadDBQueries(TransportSystem& t_system);