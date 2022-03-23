#include "serialization.h"

#include <set>

using namespace std;
using namespace transport;

std::tuple<double, int> ComputeRouteLengths(const TransportCatalogue& db, const std::vector<std::string_view>& route) 
{
    double geographic = 0.0;
    int actual = 0;

    auto prev_stop = &route[0u];
    size_t route_sz = route.size();
    for (size_t i = 1u; i < route_sz; ++i) 
    {
        const auto cur_stop = &route[i];
        const auto res_geogr = db.GetGeographicDistanceBetweenStops(*prev_stop, *cur_stop);
        geographic += (res_geogr.has_value()) ? *res_geogr : 0.0;

        const auto res_actual = db.GetActualDistanceBetweenStops(*prev_stop, *cur_stop);
        actual += (res_actual.has_value()) ? *res_actual : 0;

        prev_stop = cur_stop;
    }

    return std::tuple<double, int>(geographic, actual);
}

namespace serialize
{


void Serializer::Serialize()
{
    ofstream ofs(filename_, ios::binary);
    
    SerializeStop();
    SerializeBus();
    SerializeDistance();
    SerializeRenderSettings();
    SerializeRoutingSettings();

    transport_catalogue_serialize_.SerializeToOstream(&ofs);
}

void Serializer::Deserialize()
{
    ifstream ifs(filename_, ios::binary);

    transport_catalogue_serialize_.ParseFromIstream(&ifs);

    // MapRender
    DeserializeRenderSettings();

    // TransportCatalogue
    DeserializeStop();
    DeserializeDistance();
    DeserializeBus();

    // Router
    DeserializeRoutingSettings();
}

void Serializer::SetFileName(const std::string& filename)
{
    filename_ = filename;
}

void Serializer::SetTransportRouter(transport::Router& transport_router)
{
    transport_router_ = &transport_router;
}

// Serialize objs

void Serializer::SerializeStop()
{
    for (const auto& stop: transport_catalogue_.GetStopsInVector())
    {
        transport_catalogue_serialize::Stop stop_pb;
        stop_pb.set_name(*(stop->name));

        transport_catalogue_serialize::Coordinates coordinates_pb;
        coordinates_pb.set_lat(stop->coords.lat);
        coordinates_pb.set_lng(stop->coords.lng);

        *stop_pb.mutable_coordinates() = coordinates_pb;
        *transport_catalogue_serialize_.add_stops() = stop_pb;
    }
}

void Serializer::SerializeBus()
{
    for (const auto& bus: transport_catalogue_.GetBusesInVector())
    {
        transport_catalogue_serialize::Bus bus_pb;
        bus_pb.set_name(*(bus->name));
        bus_pb.set_roundtrip(bus->roundtrip);
        
        for (const auto stop: bus->route)
        {
            transport_catalogue_serialize::Stop stop_pb;
            stop_pb.set_name(*(stop->name));

            transport_catalogue_serialize::Coordinates coordinates_pb;
            coordinates_pb.set_lat(stop->coords.lat);
            coordinates_pb.set_lng(stop->coords.lng);

            *stop_pb.mutable_coordinates() = coordinates_pb;
            *bus_pb.add_stops() = stop_pb;
        }

        string last_stop = (bus->last_stop != nullptr) ? (*(bus->last_stop->name)) : ("");
        bus_pb.set_laststop(last_stop);

        *transport_catalogue_serialize_.add_buses() = bus_pb;
    }
}

void Serializer::SerializeDistance()
{
    for (const auto stop_pair: transport_catalogue_.GetStopPairsToDistance())
    {
        transport_catalogue_serialize::Distance distance_pb;
        distance_pb.set_from(*(stop_pair.first.first->name));
        distance_pb.set_to(*(stop_pair.first.second->name));
        distance_pb.set_distance(stop_pair.second);

        *transport_catalogue_serialize_.add_distances() = distance_pb;
    }
}

void Serializer::SerializeRenderSettings()
{
    auto render_settings = map_renderer_.GetRenderSettings();
    
    transport_catalogue_serialize::RenderSettings render_settings_pb;
    
    render_settings_pb.set_width(render_settings.width);
    render_settings_pb.set_height(render_settings.height);
    render_settings_pb.set_padding(render_settings.padding);
    render_settings_pb.set_line_width(render_settings.line_width);
    render_settings_pb.set_stop_radius(render_settings.stop_radius);
    render_settings_pb.set_bus_label_font_size(render_settings.bus_label_font_size);
    render_settings_pb.set_stop_label_font_size(render_settings.stop_label_font_size);
    render_settings_pb.set_underlayer_width(render_settings.underlayer_width);

    transport_catalogue_serialize::Point point_pb;
    point_pb.set_x(render_settings.bus_label_offset.x);
    point_pb.set_y(render_settings.bus_label_offset.y);
    *render_settings_pb.mutable_bus_label_offset() = point_pb;

    point_pb.set_x(render_settings.stop_label_offset.x);
    point_pb.set_y(render_settings.stop_label_offset.y);
    *render_settings_pb.mutable_stop_label_offset() = point_pb;

    *render_settings_pb.mutable_underlayer_color() = SerializeColor(render_settings.underlayer_color);

    for (const auto& color : render_settings.color_palette)
    {
        *render_settings_pb.add_color_palette() = SerializeColor(color);
    }

    *transport_catalogue_serialize_.mutable_render_settings() = render_settings_pb;
}

void Serializer::SerializeRoutingSettings()
{
    transport_catalogue_serialize::RoutingSettings routing_settings_pb;
    auto routing_settings = transport_router_.value()->GetRouterSettings();

    routing_settings_pb.set_bus_velocity(routing_settings.velocity);
    routing_settings_pb.set_bus_wait_time(routing_settings.wait_time);

    *transport_catalogue_serialize_.mutable_routing_settings() = routing_settings_pb;
}

transport_catalogue_serialize::Color Serializer::SerializeColor(const svg::Color& color)
{
    transport_catalogue_serialize::Color color_pb;
    transport_catalogue_serialize::Rgba rgba_pb;

    if (std::holds_alternative<svg::Rgb>(color))
    {
        svg::Rgb rgb = std::get<svg::Rgb>(color);
        rgba_pb.set_blue(rgb.blue);
        rgba_pb.set_green(rgb.green);
        rgba_pb.set_red(rgb.red);
        *color_pb.mutable_rgba() = rgba_pb;
    }
    else if (std::holds_alternative<svg::Rgba>(color))
    {
        svg::Rgba rgba = std::get<svg::Rgba>(color);
        rgba_pb.set_blue(rgba.blue);
        rgba_pb.set_green(rgba.green);
        rgba_pb.set_red(rgba.red);
        rgba_pb.set_opacity(rgba.opacity);
        color_pb.set_is_rgba(true);
        *color_pb.mutable_rgba() = rgba_pb;
    }
    else if(std::holds_alternative<std::string>(color))
    {
        color_pb.set_name(std::get<std::string>(color));
    }
    return color_pb;
}

// Deserialize objs

void Serializer::DeserializeStop()
{
    for (int i = 0; i < transport_catalogue_serialize_.stops().size(); ++i)
    {
        auto stop_pb = transport_catalogue_serialize_.stops(i);
        
        domain::Stop stop(
            move(string(stop_pb.name())),
            stop_pb.coordinates().lat(),
            stop_pb.coordinates().lng()
        );

        transport_catalogue_.AddStop(move(stop));        
    }
}

void Serializer::DeserializeBus()
{
    // Requirements:
    // - name
    // - route (vector of stops)
    // - unique
    // - actual
    // - geo
    // - last stop
    // - !Stops are initialized in transport_catalogue

    for (int i = 0; i < transport_catalogue_serialize_.buses().size(); ++i)
    {
        auto bus_pb = transport_catalogue_serialize_.buses(i);
        
        vector<string_view> route_names;
        vector<domain::StopPointer> route;
        set<string_view> unique_stops;
        for (const auto& stop_pb: bus_pb.stops())
        {
            route_names.push_back(stop_pb.name());
            route.push_back(transport_catalogue_.FindStop(stop_pb.name()));
            unique_stops.emplace(stop_pb.name());
        }
        
        auto [geographic, actual] = ComputeRouteLengths(transport_catalogue_, route_names);

        domain::Bus bus(
            move(string(bus_pb.name())),
            move(route),
            unique_stops.size(),
            actual,
            geographic,
            bus_pb.roundtrip(),
            transport_catalogue_.FindStop(bus_pb.laststop())
        );

        transport_catalogue_.AddBus(move(bus));
    }
}

void Serializer::DeserializeDistance()
{
    for (int i = 0; i < transport_catalogue_serialize_.distances().size(); ++i)
    {
        transport_catalogue_serialize::Distance distance_pb = transport_catalogue_serialize_.distances(i);
        transport_catalogue_.SetDistanceBetweenStops(distance_pb.from(), distance_pb.to(), distance_pb.distance());
    }
}

void Serializer::DeserializeRenderSettings()
{
    renderer::RenderingSettings render_settings;
    auto render_settings_pb = transport_catalogue_serialize_.render_settings();

    render_settings.width = render_settings_pb.width();
    render_settings.height = render_settings_pb.height();
    render_settings.padding = render_settings_pb.padding();
    render_settings.line_width = render_settings_pb.line_width();
    render_settings.stop_radius = render_settings_pb.stop_radius();
    render_settings.bus_label_font_size = render_settings_pb.bus_label_font_size();
    render_settings.stop_label_font_size = render_settings_pb.stop_label_font_size();
    render_settings.underlayer_width = render_settings_pb.underlayer_width();

    render_settings.bus_label_offset = { render_settings_pb.bus_label_offset().x(), render_settings_pb.bus_label_offset().y() };
    render_settings.stop_label_offset = { render_settings_pb.stop_label_offset().x(), render_settings_pb.stop_label_offset().y() };

    render_settings.underlayer_color = DeserializeColor(render_settings_pb.underlayer_color());

    for (const auto& color_pb : render_settings_pb.color_palette())
    {
        render_settings.color_palette.push_back(DeserializeColor(color_pb));
    }
    map_renderer_.SetSettings(move(render_settings));
}

void Serializer::DeserializeRoutingSettings()
{
    auto routing_settings_pb = transport_catalogue_serialize_.routing_settings();
    transport_router_.value()->SetSettings(routing_settings_pb.bus_wait_time(), routing_settings_pb.bus_velocity());

    transport_router_.value()->FillGraph(transport_catalogue_);
    transport_router_.value()->BuildGraph();
    transport_router_.value()->BuildRouter();
}

svg::Color Serializer::DeserializeColor(const transport_catalogue_serialize::Color& color_pb)
{
    if (!color_pb.name().empty())
    {
        return color_pb.name();
    }
    else if (color_pb.is_rgba())
    {
        return svg::Rgba(color_pb.rgba().red(), color_pb.rgba().green(), color_pb.rgba().blue(), color_pb.rgba().opacity());
    }
    return svg::Rgb(color_pb.rgba().red(), color_pb.rgba().green(), color_pb.rgba().blue());
}

}