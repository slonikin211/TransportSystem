#pragma once

#include "router.h"

#include <string>
#include <string_view>

namespace transport_system
{

struct EdgeInfo {
    graph::Edge<double> edge;

    std::string_view name;
    int span_count = -1;
    double time = 0.0;
};

struct RouteItemWait {
    std::string_view stop_name;
    double time = 0.0;
};

struct RouteItemBus {
    std::string_view bus_name;
    int span_count;
    double time = 0.0;
};

struct RouteItem {
    std::optional<RouteItemWait> wait_item;
    std::optional<RouteItemBus> bus_item;
};

struct OutRouteinfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};


class RouterSetting
{
private:
    static constexpr double TO_MINUTES = 3.6 / 60.0;
    
    using Graph = graph::DirectedWeightedGraph<double>;
    using RouterGraph = graph::TransportRouter<double>;

    struct Settings {
        double bus_wait_time = 6u;
        double bus_velocity = 40u;
    };

    struct Vertexes {
        size_t start_wait;
        size_t end_wait;
    };

public:
    RouterSetting() = default;
    explicit RouterSetting(const size_t graph_size);

    void SetSettings(const double bus_wait_time, const double bus_velocity);

    void AddWaitEdge(const std::string_view stop_name);
    void AddBusEdge(const std::string_view stop_from, const std::string_view stop_to, const std::string_view bus_name, const int span_count, const double dist);
    void AddStop(const std::string_view stop_name);

    void BuildGraph();
    void BuildRouter();

    std::optional<OutRouteinfo> GetRouteInfo(const std::string_view from, const std::string_view to) const;

private:
    std::optional<Graph> graph_ = std::nullopt;
    std::optional<RouterGraph> router_ = std::nullopt;

    Settings settings_;

    std::unordered_map<std::string_view, Vertexes, std::hash<std::string_view>> stop_to_vertex_id_;
    std::vector<EdgeInfo> edges_;

    void AddEdgesToGraph();
    std::vector<RouteItem> MakeItemsByEdgeIds(const std::vector<graph::EdgeId>& edge_ids) const;
};

}

