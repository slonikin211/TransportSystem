#include "transport_router.h"

namespace transport 
{
	using namespace std;
	using namespace domain;

	Router::Router(const size_t graph_size) : graph_(graph_size) {}

	void Router::SetSettings(const double bus_wait_time, const double bus_velocity) 
	{
		settings_ = { bus_wait_time, bus_velocity };
	}

	void Router::AddWaitEdge(const string_view stop_name) 
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
		edges_.push_back(move(new_edge));
	}

	void Router::AddBusEdge(const BusEdgeInfo& bus_edge_info) {
		EdgeInfo new_edge{
			{
				stop_to_vertex_id_[bus_edge_info.stop_from].end_wait,
				stop_to_vertex_id_[bus_edge_info.stop_to].start_wait,
				bus_edge_info.dist / settings_.velocity * TO_MINUTES
			},
			bus_edge_info.bus_name,
			(int)bus_edge_info.span_count,
			bus_edge_info.dist / settings_.velocity * TO_MINUTES
		};
		edges_.push_back(move(new_edge));
	}

	void Router::AddStop(const string_view stop_name) 
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
			graph_ = move(Graph(stop_to_vertex_id_.size() * 2u));
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

	void Router::FillGraph(const TransportCatalogue& db)
	{
		for (const StopPointer stop : db.GetStopsInVector()) 
        {
			std::string_view stop_name(*stop.get()->name.get());
			AddStop(stop_name);
			AddWaitEdge(stop_name);
		}

		for (const BusPointer bus : db.GetBusesInVector()) 
        {
			const std::string_view bus_name = *bus->name;
			for (size_t i = 0u; i < bus->route.size() - 1u; ++i) {
				const std::string_view stop_name_from = *bus->route[i]->name;

				double prev_actual = 0.0;
				std::string_view prev_stop_name = stop_name_from;

				for (size_t j = i + 1u; j < bus->route.size(); ++j) 
                {
					const std::string_view stop_name_to = *bus->route[j]->name;
					optional<double> actual = db.GetActualDistanceBetweenStops(prev_stop_name, stop_name_to);
					if (actual)
					{
						AddBusEdge({
							stop_name_from,
							stop_name_to,
							bus_name,
							j - i,
							prev_actual + actual.value()
						});
						prev_stop_name = stop_name_to;
						prev_actual += actual.value();
					}
				}
			}
		}
	}

	optional<RouteInfo> Router::GetRouteInfo(const string_view from, const string_view to) const {
		const auto route = router_->BuildRoute(
			stop_to_vertex_id_.at(from).start_wait,
			stop_to_vertex_id_.at(to).start_wait
		);
		if (!route) 
		{
			return nullopt;
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

	vector<RouteItem> Router::MakeItemsByEdgeIds(const vector<graph::EdgeId>& edge_ids) const 
	{
		vector<RouteItem> result;
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
			result.push_back(move(tmp));
		}
		return result;
	}
}
