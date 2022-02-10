#pragma once

#include "domain.h"

#include <string>
#include <vector>
#include <string_view>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <optional>
#include <memory>

namespace transport 
{

	class TransportCatalogue 
    {
	private:
		using StopsPair = std::pair<domain::StopPointer, domain::StopPointer>;

		class StopsPairHasher 
        {
		public:
			std::size_t operator()(const StopsPair& stops_pair) const;

		private:
			std::hash<const void*> hash_;
		};

	public:
		void AddBus(domain::Bus&& bus);
		void AddStop(domain::Stop&& stop);
		void SetDistanceBetweenStops(const std::string_view first, const std::string_view second, double distance);

		domain::BusPointer FindBus(const std::string_view name)  const;
		domain::StopPointer FindStop(const std::string_view name) const;

		std::optional<double> GetActualDistanceBetweenStops(const std::string_view stop1_name, const std::string_view stop2_name) const;
		std::optional<double> GetGeographicDistanceBetweenStops(const std::string_view stop1_name, const std::string_view stop2_name) const;
		
        const std::unordered_set<domain::BusPointer>* GetPassingBusesByStop(domain::StopPointer stop) const;
		const std::vector<domain::BusPointer> GetBusesInVector() const;
		const std::vector<domain::StopPointer> GetStopsInVector() const;

	private:
		std::deque<std::shared_ptr<domain::Stop>> stops_;
		std::deque<std::shared_ptr<domain::Bus>> buses_;

		std::unordered_map<std::string_view, domain::BusPointer, std::hash<std::string_view>> name_to_bus_;
		std::unordered_map<std::string_view, domain::StopPointer, std::hash<std::string_view>> name_to_stop_;

		std::unordered_map<StopsPair, int, StopsPairHasher> stops_pair_to_distance_;
		std::unordered_map<domain::StopPointer, std::unordered_set<domain::BusPointer>, std::hash<domain::StopPointer>> stop_to_passing_buses_;

		void AddToStopPassingBuses(const std::vector<domain::StopPointer>& stops, const std::string_view bus_name);
	};
}
