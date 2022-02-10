#include "transport_router.h"

namespace transport 
{
	Router::Router(const size_t graph_size) : graph_(graph_size) {}

	void Router::SetSettings(const double bus_wait_time, const double bus_velocity) 
	{
		settings_ = { bus_wait_time, bus_velocity };
	}

	void Router::AddWaitEdge(const std::string_view stop_name) 
	{
		EdgeInfo new_edge{
			{
				stop_to_vertex_id_[stop_name].start_wait,
				stop_to_vertex_id_[stop_name].end_wait,
				settings_.wait_time
			},
			stop_name,
			-1,
			settings_.wait_time
		};
		edges_.push_back(std::move(new_edge));
	}

	void Router::AddBusEdge(const std::string_view stop_from, const std::string_view stop_to, const std::string_view bus_name, const int span_count, const int dist) {
		EdgeInfo new_edge{
			{
				stop_to_vertex_id_[stop_from].end_wait,
				stop_to_vertex_id_[stop_to].start_wait,
				dist / settings_.velocity * TO_MINUTES
			},
			bus_name,
			span_count,
			dist / settings_.velocity * TO_MINUTES
		};
		edges_.push_back(std::move(new_edge));
	}

	void Router::AddStop(const std::string_view stop_name) 
	{
		if (!stop_to_vertex_id_.count(stop_name)) 
		{
			size_t sz = stop_to_vertex_id_.size();
			stop_to_vertex_id_[stop_name] = { sz * 2u, sz * 2u + 1u };
		}
	}

	void Router::BuildGraph() 
	{
		if (!graph_) 
		{
			graph_ = std::move(Graph(stop_to_vertex_id_.size() * 2u));
		}
		AddEdgesToGraph();
	}

	void Router::BuildRouter() 
	{
		if (!router_ && graph_) 
		{
			router_.emplace(RouterG(*graph_));
		}
	}

	std::optional<RouteInfo> Router::GetRouteInfo(const std::string_view from, const std::string_view to) const {
		const auto route = router_->BuildRoute(
			stop_to_vertex_id_.at(from).start_wait,
			stop_to_vertex_id_.at(to).start_wait
		);
		if (!route) 
		{
			return std::nullopt;
		}

		return RouteInfo{
			route->weight,
			MakeItemsByEdgeIds(route->edges)
		};
	}

	void Router::AddEdgesToGraph() 
	{
		for (auto& edge_info : edges_) 
		{
			graph_->AddEdge(edge_info.edge);
		}
	}

	std::vector<RouteItem> Router::MakeItemsByEdgeIds(const std::vector<graph::EdgeId>& edge_ids) const 
	{
		std::vector<RouteItem> result;
		result.reserve(edge_ids.size());

		for (const auto id : edge_ids) 
		{
			const EdgeInfo& edge_info = edges_[id];
			RouteItem tmp;
			if (edge_info.span_count == -1) 
			{
				tmp.wait_item = {
					edge_info.name,
					edge_info.time
				};
			}
			else 
			{
				tmp.bus_item = {
					edge_info.name,
					edge_info.span_count,
					edge_info.time
				};
			}
			result.push_back(std::move(tmp));
		}
		return result;
	}
}
