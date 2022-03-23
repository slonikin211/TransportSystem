#include "geo.h"
#include "transport_catalogue.h"

#include <utility>
#include <set>
#include <cmath>

namespace transport 
{
	using namespace domain;

	size_t TransportCatalogue::StopsPairHasher::operator()(const StopsPair& stops_pair) const 
    {
		return {
			hash_(stops_pair.first.get()) +
			hash_(stops_pair.second.get()) * 37 * 37
		};
	}

	void TransportCatalogue::AddBus(Bus&& bus) 
    {
		buses_.push_back(std::make_shared<Bus>(std::move(bus)));
		const auto bus_ptr = buses_.back().get();
		name_to_bus_[*bus_ptr->name.get()] = buses_.back();

		AddToStopPassingBuses(bus_ptr->route, *bus_ptr->name.get());
	}

	void TransportCatalogue::AddStop(Stop&& stop) 
    {
		if (name_to_stop_.count(*stop.name.get())) { return; }

		stops_.push_back(std::make_shared<Stop>(std::move(stop)));
		const auto stop_ptr = stops_.back().get();
		name_to_stop_[*stop_ptr->name.get()] = stops_.back();

		stops_pair_to_distance_[{ stops_.back(), stops_.back() }] = 0;
	}

	void TransportCatalogue::SetDistanceBetweenStops(const std::string_view first, const std::string_view second, double distance) 
    {
		StopPointer stop_X = FindStop(first);
		StopPointer stop_To = FindStop(second);

		stops_pair_to_distance_[{stop_X, stop_To}] = distance;
		StopsPair tmp_pair = { stop_To, stop_X };

		if (stops_pair_to_distance_.count(tmp_pair) == 0u) {
			stops_pair_to_distance_[move(tmp_pair)] = distance;
		}
	}

	BusPointer TransportCatalogue::FindBus(const std::string_view name) const 
    {
		return (name_to_bus_.count(name) ? name_to_bus_.at(name) : nullptr);
	}

	StopPointer TransportCatalogue::FindStop(const std::string_view name) const 
    {
		return (name_to_stop_.count(name) ? name_to_stop_.at(name) : nullptr);
	}

	std::optional<double> TransportCatalogue::GetActualDistanceBetweenStops(const std::string_view stop1_name, const std::string_view stop2_name) const 
    {
		StopPointer first_stop = FindStop(stop1_name);
		StopPointer second_stop = FindStop(stop2_name);
		if (first_stop.get() == nullptr || second_stop.get() == nullptr) 
        {
			return {};
		}
		const StopsPair tmp_pair = { first_stop, second_stop };

		return (stops_pair_to_distance_.count(tmp_pair) ? stops_pair_to_distance_.at(tmp_pair) : std::optional<double>{});
	}

	std::optional<double> TransportCatalogue::GetGeographicDistanceBetweenStops(const std::string_view stop1_name, const std::string_view stop2_name) const 
    {
		StopPointer first_stop = FindStop(stop1_name);
		StopPointer second_stop = FindStop(stop2_name);
		if (first_stop == nullptr || second_stop == nullptr) 
        {
			return {};
		}

		return first_stop->GetGeographicDistanceTo(second_stop);
	}

	const std::unordered_set<BusPointer>* TransportCatalogue::GetPassingBusesByStop(StopPointer stop) const 
    {
		return ((stop_to_passing_buses_.count(stop)) ? &stop_to_passing_buses_.at(stop) : nullptr);
	}

	const std::vector<BusPointer> TransportCatalogue::GetBusesInVector() const 
    {
		return std::vector<BusPointer>(buses_.begin(), buses_.end());
	}

	const std::vector<StopPointer> TransportCatalogue::GetStopsInVector() const 
    {
		return std::vector<StopPointer>(stops_.begin(), stops_.end());
	}

	const std::unordered_map<TransportCatalogue::StopsPair, int, TransportCatalogue::StopsPairHasher> TransportCatalogue::GetStopPairsToDistance() const
	{
		return std::unordered_map<StopsPair, int, StopsPairHasher>(stops_pair_to_distance_.begin(), stops_pair_to_distance_.end());
	}

	void TransportCatalogue::AddToStopPassingBuses(const std::vector<StopPointer>& stops, const std::string_view bus_name) 
    {
		BusPointer bus = FindBus(bus_name);
		for (size_t i = 0u; i < stops.size(); ++i) 
        {
			stop_to_passing_buses_[stops[i]].insert(bus);
		}
	}
}
