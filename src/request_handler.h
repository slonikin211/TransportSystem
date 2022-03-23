#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "serialization.h"

#include <optional>
#include <unordered_set>
#include <tuple>
#include <string>
#include <string_view>
#include <memory>

namespace request_handler 
{
	class RequestHandler 
	{
	private:
		enum class SeparatorType 
		{
			DASH,
			GREATER_THAN,
			NO_HAVE
		};

	public:
		RequestHandler(transport::TransportCatalogue& db, renderer::MapRenderer& mr);

		void AddBus(domain::Bus&& bus);
		void AddStop(domain::Stop&& stop);

		void SetDistanceBetweenStops(const std::string_view raw_query);
		void SetDistanceBetweenStops(const std::string_view first, const std::string_view second, double distance);

		domain::BusPointer FindBus(const std::string_view name) const;
		domain::StopPointer FindStop(const std::string_view name) const;

		const std::vector<domain::BusPointer> GetBusesInVector() const;
		const std::vector<domain::StopPointer> GetStopsInVector() const;

		std::optional<domain::BusInfo> GetBusInfo(const std::string_view bus_name) const;
		std::optional<domain::StopInfo> GetStopInfo(const std::string_view stop_name) const;

		const std::unordered_set<domain::BusPointer>* GetBusesByStop(const std::string_view stop_name) const;
		std::tuple<double, int> ComputeRouteLengths(const std::vector<std::string_view>& routh) const;
		std::vector<domain::StopPointer> StopsToStopPointer(const std::vector<std::string_view>& stops) const;

		std::optional<double> GetActualDistanceBetweenStops(const std::string_view stop1_name, const std::string_view stop2_name) const;

		svg::Document RenderMap() const;
		void SetRenderSettings(renderer::RenderingSettings&& settings);

		void SetRoutingSettings(const double bus_wait_time, const double bus_velocity);
		void AddStopToRouter(const std::string_view name);
		void AddWaitEdgeToRouter(const std::string_view stop_name);
		void AddBusEdgeToRouter(const std::string_view stop_from, const std::string_view stop_to, const std::string_view bus_name, const size_t span_count, const double dist);
		void FillRouter();
		void BuildRouter();
		std::optional<transport::RouteInfo> GetRouteInfo(const std::string_view from, const std::string_view to) const;

		void SetSerializationSettings(const std::string& filename);
		void Serialize();
		void Deserialize();
	private:
		transport::TransportCatalogue& db_;
		renderer::MapRenderer& mr_;
		transport::Router rt_;
		serialize::Serializer sz_;

		std::tuple<std::string, std::size_t> QueryGetName(const std::string_view str) const;
		std::tuple<std::string, std::string> SplitIntoLengthStop(std::string&& str) const;
		std::tuple<std::vector<std::string>, SeparatorType> SplitIntoWordsBySeparator(const std::string_view str) const;
		std::tuple<std::vector<std::string_view>, int> WordsToRoute(const std::vector<std::string>& words, SeparatorType separator) const;
	};
}
