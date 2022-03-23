#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_set>
#include <memory>
#include <optional>

namespace domain 
{
	struct Bus;
	using BusPointer = std::shared_ptr<Bus>;

	struct Stop;
	using StopPointer = std::shared_ptr<Stop>;

	struct Bus 
    {
		Bus(std::string&& name, std::vector<StopPointer>&& route, int unique, 
            int actual, double geo, bool roundtrip, StopPointer last_stop = nullptr);

		Bus& operator=(const Bus& bus) = default;

		std::shared_ptr<std::string> name;
		std::vector<StopPointer> route;
		int unique_stops = 0;
		int route_actual_length = 0;
		double route_geographic_length = 0.0;
		bool roundtrip;
		StopPointer last_stop;
	};

	struct Stop 
    {
		Stop(std::string&& name, double lat, double lng);

		double GetGeographicDistanceTo(StopPointer stop_to) const;

		std::shared_ptr<std::string> name;
		geo::Coordinates coords = {0.0, 0.0};
	};

	struct BusInfo 
    {
		std::string_view name;
		int stops_on_route = 0;
		int unique_stops = 0;
		int routh_actual_length = 0;
		double curvature = 0.0;
	};

	struct StopInfo 
    {
		std::string_view name;
		const std::unordered_set<BusPointer>* passing_buses;
	};

}
