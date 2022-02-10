#pragma once

#include "router.h"
#include "transport_catalogue.h"

#include <utility>
#include <vector>
#include <unordered_map>
#include <string>
#include <string_view>
#include <optional>
#include <functional>

namespace transport 
{
	struct EdgeInfo 
    {
		graph::Edge<double> edge;

		std::string_view name;
		int span_count = -1;
		double time = 0.0;
	};

	struct RouteItemWait 
    {
		std::string_view stop_name;
		double time = 0.0;
	};

	struct RouteItemBus 
    {
		std::string_view bus_name;
		int span_count = -1;        // CHECK THIS OUT
		double time = 0.0;
	};

	struct RouteItem 
    {
		std::optional<RouteItemWait> wait_item;
		std::optional<RouteItemBus> bus_item;
	};

	struct RouteInfo 
    {
		double total_time = 0.0;
		std::vector<RouteItem> items;
	};

	class Router 
    {
	private:
		static constexpr double TO_MINUTES = (3.6 / 60.0);

		using Graph = graph::DirectedWeightedGraph<double>;
		using RouterG = graph::TransportRouter<double>;

		struct Settings 
        {
			double wait_time = 6.0;
			double velocity = 40.0;
		};

		struct Vertexes 
        {
			size_t start_wait;
			size_t end_wait;
		};

		struct BusEdgeInfo
		{
			std::string_view stop_from; 
			std::string_view stop_to;
			std::string_view bus_name; 
			size_t span_count;
			double dist;
		};

	public:
		Router() = default;
		explicit Router(const size_t graph_size);

		void SetSettings(const double bus_wait_time, const double bus_velocity);
		void AddWaitEdge(const std::string_view stop_name);
		void AddBusEdge(const BusEdgeInfo& bus_edge_info);
		void AddStop(const std::string_view stop_name);

		void BuildGraph();
		void BuildRouter();

		void FillGraph(const TransportCatalogue& db);

		std::optional<RouteInfo> GetRouteInfo(const std::string_view from, const std::string_view to) const;

	private:
		Settings settings_;

		std::optional<Graph> graph_ = std::nullopt;
		std::optional<RouterG> router_ = std::nullopt;

		std::unordered_map<std::string_view, Vertexes, std::hash<std::string_view>> stop_to_vertex_id_;
		std::vector<EdgeInfo> edges_;

		void AddEdgesToGraph();
		std::vector<RouteItem> MakeItemsByEdgeIds(const std::vector<graph::EdgeId>& edge_ids) const;
	};
}
