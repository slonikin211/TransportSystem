#include "domain.h"

#include <utility>

namespace domain 
{
	Bus::Bus(std::string&& name, std::vector<StopPointer>&& route, int unique, 
            int actual, double geo, bool roundtrip, StopPointer last_stop) : 
        name(std::make_shared<std::string>(std::move(name))), 
        route(std::vector<StopPointer>(std::move(route))), 
        unique_stops(unique),
        route_actual_length(actual),
        route_geographic_length(geo),
        roundtrip(roundtrip),
        last_stop(last_stop) {}


	Stop::Stop(std::string&& name, double lat, double lng) 
        : name(std::make_shared<std::string>(std::move(name))), coords({lat, lng}) {}

	double Stop::GetGeographicDistanceTo(StopPointer stop_to) const {
		return geo::ComputeDistance(
            { coords.lat, coords.lng }, 
            { stop_to.get()->coords.lat, stop_to.get()->coords.lng });
	}
}
