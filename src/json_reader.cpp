#include "json_reader.h"
#include "map_renderer.h"
#include "json_builder.h"
#include "transport_router.h"

#include <iostream>
#include <sstream>
#include <string>
#include <deque>
#include <memory>
#include <fstream>

using namespace std;
using namespace svg;
using namespace json;
using namespace transport_system;
using namespace request_handler;
using namespace request_handler::query;     // cannot to delete the namespace
using namespace request_handler::init;      // cannot to delete the namespace
using namespace geo;
using namespace obj;
using namespace info;
using namespace map_renderer;
using namespace map_renderer::detail;       // cannot to delete the namespace


namespace render
{
    namespace
    {
        Color GetColor(const Node& node)
        {
            if (node.IsString()) {
                return node.AsString();
            }
            else if (node.IsArray()) {
                if (node.AsArray().size() == 3u) {   // rgb
                    return Rgb{
                        static_cast<uint8_t>(node.AsArray().at(0u).AsInt()),
                        static_cast<uint8_t>(node.AsArray().at(1u).AsInt()),
                        static_cast<uint8_t>(node.AsArray().at(2u).AsInt())
                    };
                }
                else {  // rgba
                    return Rgba{
                        static_cast<uint8_t>(node.AsArray().at(0u).AsInt()),
                        static_cast<uint8_t>(node.AsArray().at(1u).AsInt()),
                        static_cast<uint8_t>(node.AsArray().at(2u).AsInt()),
                        node.AsArray().at(3u).AsDouble()
                    };
                }
            }
            // Need to throws error
            return ""s;
        }
    }

    MapRendererSettings InitMapRendererSettings(const Dict& root) 
    {
        MapRendererSettings settings;
        settings.width = root.at("width"s).AsDouble();
        settings.height = root.at("height"s).AsDouble();

        settings.padding = root.at("padding"s).AsDouble();
        settings.line_width = root.at("line_width"s).AsDouble();
        settings.stop_radius = root.at("stop_radius"s).AsDouble();

        settings.bus_label_font_size = root.at("bus_label_font_size"s).AsDouble();
        auto bus_label_offset_array = root.at("bus_label_offset"s).AsArray();
        settings.bus_label_offset = {bus_label_offset_array.front().AsDouble(), bus_label_offset_array.back().AsDouble(), };

        settings.stop_label_font_size = root.at("stop_label_font_size"s).AsDouble();
        auto stop_label_offset_array = root.at("stop_label_offset"s).AsArray();
        settings.stop_label_offset = {stop_label_offset_array.front().AsDouble(), stop_label_offset_array.back().AsDouble(), };

        settings.underlayer_color = GetColor(root.at("underlayer_color"s));
        settings.underlayer_width = root.at("underlayer_width"s).AsDouble();
        
        auto color_palette = root.at("color_palette"s).AsArray();
        for (const auto& color: color_palette) {
            settings.color_palette.push_back(GetColor(color));
        }
        return settings;
    }
    
}

namespace json::read
{
    namespace detail
    {
        namespace init
        {
            namespace
            {
                query::InitStop MakeQueryInitStop(const Dict& query)
                {
                    const string& type = query.at("type"s).AsString();

                    if (type != "Stop"s) {
                        throw logic_error("Invalid type for MakeQueryInitStop");
                    }

                    const string& name = query.at("name"s).AsString();
                    double lat = query.at("latitude"s).AsDouble();
                    double lng = query.at("longitude"s).AsDouble();

                    const Dict& road_distances = query.at("road_distances"s).AsDict();
                    map<string, double> road_distances_res;

                    for (const auto stop_info: road_distances) {
                        road_distances_res.insert({stop_info.first, stop_info.second.AsDouble()});
                    }

                    return {
                        name,
                        {lat, lng},
                        move(road_distances_res)
                    };
                }
            
                query::InitBus MakeQueryInitBus(const Dict& query)
                {
                    const string& type = query.at("type"s).AsString();

                    if (type != "Bus"s) {
                        throw logic_error("Invalid type for MakeQueryInitBus");
                    }

                    const string& name = query.at("name"s).AsString();
                    bool is_roundtrip = query.at("is_roundtrip"s).AsBool();

                    const Array& stops = query.at("stops"s).AsArray();
                    vector<string> stops_res;

                    stops_res.reserve(stops.size());
                    for (const auto stop: stops) {
                        stops_res.push_back(stop.AsString());
                    }

                    return {
                        name,
                        move(stops_res),
                        is_roundtrip
                    };
                }
            }

            void ProceedInitStop(TransportSystem& system, const Dict& query)
            {
                const string& type = query.at("type"s).AsString();
                if (type != "Stop"s) {
                    return;
                }
                request_handler::init::InitStop(system, MakeQueryInitStop(query));
            }

            void ProceedInitQuery(TransportSystem& system, const Dict& query)
            {
                const string& type = query.at("type"s).AsString();
                if (type == "Stop"s) {
                    // Init was at previos finction called ProceedInitStop, so here is just linking
                    request_handler::init::InitStopsConnections(system, MakeQueryInitStop(query));                  
                }
                else if (type == "Bus"s) {
                    request_handler::init::InitBus(system, MakeQueryInitBus(query));
                }
            }

        }
        void ProceedInitQueries(TransportSystem& system, const Array& init, RouterSetting& rs)
        {
            // Init stops first
            for (const auto& query: init) {
                init::ProceedInitStop(system, query.AsDict());
            }

            // Init all queries (with link stops)
            for (const auto& query: init) {
                init::ProceedInitQuery(system, query.AsDict());
            }

            // Fill transport router
            request_handler::init::FillTransportRouter(system, rs);
        }
    

        namespace process
        {
            OutRoute ProceedRouteQuery(const RouterSetting& router_setting, const Dict& q)
            {
                if (q.at("type"s) != "Route"s) {  // just to make the below code safer
                    throw std::logic_error("It is not a route query (ProceedRouteQuery)");
                } 

                OutRoute result;
                result.id = q.at("id"s).AsInt();

                std::optional<transport_system::OutRouteinfo> route_info = request_handler::process::GetRouteInfo(
                    router_setting, 
                    q.at("from").AsString(),
                    q.at("to").AsString()
                );

                if (!route_info) 
                {
                    result.additional_data = "not found"s;
                    return result;
                }

                result.route_info = std::move(route_info);
                return result;
            }

            OutMap ProceedRenderQuery(const TransportSystem& system, const Dict& q, const MapRendererSettings& settings)
            {
                if (q.at("type"s) != "Map"s) {  // just to make the below code safer
                    throw std::logic_error("It is not a map query (ProceedRenderQuery)");
                } 
                
                std::ostringstream os;
                request_handler::process::GetSVGDocument(system, settings, os);

                OutMap result;
                result.id = q.at("id"s).AsInt();
                result.os = std::move(os);
                return result;
            }

            OutBus ProceedBusQuery(const TransportSystem& system, const Dict& q)
            {
                if (q.at("type"s) != "Bus"s) {  // just to make the below code safer
                    throw std::logic_error("It is not a bus query (ProceedBusQuery)");
                } 
                BusInfo info = request_handler::process::GetBusInfo(system, {
                    q.at("id"s).AsInt(),
                    q.at("type"s).AsString(),
                    q.at("name"s).AsString()
                });
                
                OutBus result;
                result.id = q.at("id"s).AsInt();

                // Bus not found
                if (info.bus == nullptr) {
                    result.additional_data = "not found"s;
                    return result;
                }
                
                result.curvature = info.curvature;
                result.route_length = info.route_length;
                result.stop_count = info.amount_of_stops;
                result.unique_stop_count = info.amount_of_unique_stops;
                
                return result;
            }

            OutStop ProceedStopQuery(const TransportSystem& system, const Dict& q)
            {
                if (q.at("type"s) != "Stop"s) {  // just to make the below code safer
                    throw std::logic_error("It is not a stop query (ProceedStopQuery)"s);
                }
                StopInfo info = request_handler::process::GetStopInfo(system, {
                    q.at("id"s).AsInt(),
                    q.at("type"s).AsString(),
                    q.at("name"s).AsString()
                });
                
                OutStop result;
                result.id = q.at("id"s).AsInt();

                if (info.stop == nullptr) {
                    result.additional_data = "not found"s;
                    return result;
                }

                for (auto bus: info.buses) {
                    result.buses.insert(bus->name);
                }
                return result;
            }

            deque<unique_ptr<OutQuery>> ProceedAfterInitQueries(const TransportSystem& system, const Array& queries, 
                        const MapRendererSettings& map_settings, const RouterSetting& router_setting)
            {
                deque<unique_ptr<OutQuery>> result;

                for (const auto& query: queries) {
                    const auto& q = query.AsDict();
                    if (q.at("type"s) == "Bus"s) {
                        result.push_back(move(make_unique<OutBus>(ProceedBusQuery(system, q))));
                    }
                    else if (q.at("type"s) == "Stop"s) {
                        result.push_back(move(make_unique<OutStop>(ProceedStopQuery(system, q))));
                    }
                    else if (q.at("type"s) == "Map"s) {
                        result.push_back(move(make_unique<OutMap>(ProceedRenderQuery(system, q, map_settings))));
                    }
                    else if (q.at("type"s) == "Route"s) {
                        result.push_back(move(make_unique<OutRoute>(ProceedRouteQuery(router_setting, q))));
                    }
                }

                return result;
            }
        }

        namespace print
        {
            namespace
            {
                void PrintAnswer(request_handler::query::OutQuery& query, std::ostream& os)
                {
                    if (query.additional_data == "not found"s) {
                        json::Print(
                            json::Document{
                                json::Builder{}
                                    .StartDict()
                                        .Key("request_id"s).Value(query.id)
                                        .Key("error_message"s).Value("not found"s)
                                    .EndDict()
                                .Build()
                            }, os
                        );
                        return;
                    }

                    if (const OutBus* bus_info = dynamic_cast<const OutBus*>(&query)) {
                        json::Print(
                            json::Document{
                                json::Builder{}
                                    .StartDict()
                                        .Key("request_id"s).Value(query.id)
                                        .Key("curvature"s).Value(bus_info->curvature)
                                        .Key("route_length"s).Value(bus_info->route_length)
                                        .Key("stop_count"s).Value(bus_info->stop_count)
                                        .Key("unique_stop_count"s).Value(bus_info->unique_stop_count)
                                    .EndDict()
                                .Build()
                            }, os
                        );
                    }
                    else if (const OutStop* stop_info = dynamic_cast<const OutStop*>(&query)) {
                        json::Print(
                            json::Document{
                                json::Builder{}
                                    .StartDict()
                                        .Key("request_id"s).Value(query.id)
                                        .Key("buses"s).Value(Array{stop_info->buses.begin(), stop_info->buses.end()})
                                    .EndDict()
                                .Build()
                            }, os
                        );
                    }
                    else if (const OutMap* map_info = dynamic_cast<const OutMap*>(&query)) {
                        std::string data = map_info->os.str();
                        json::Print(
                            json::Document{
                                json::Builder{}
                                    .StartDict()
                                        .Key("request_id"s).Value(query.id)
                                        .Key("map"s).Value(Node(data))
                                    .EndDict()
                                .Build()
                            }, os
                        );
                    }
                    else if (const OutRoute* route_info = dynamic_cast<const OutRoute*>(&query)) {
                       json::Array arr;
                       arr.reserve(route_info->route_info->items.size());
                       
                       for (const auto item: route_info->route_info->items) {
                           if (item.wait_item) {
                                json::Dict dict = {
                                    { "type"s, json::Node(std::move("Wait"s)) },
                                    { "stop_name"s, json::Node(std::move(std::string(item.wait_item->stop_name))) },
                                    { "time"s, json::Node(item.wait_item->time) }
                                };
                                arr.push_back(dict);
                           }
                           else {
                                json::Dict dict = {
                                    { "type"s,       json::Node(std::move("Bus"s))         },
                                    { "bus"s,        json::Node(std::move(std::string(item.bus_item->bus_name)))       },
                                    { "span_count"s, json::Node(item.bus_item->span_count) },
                                    { "time"s,       json::Node(item.bus_item->time)       }
                                };
                                arr.push_back(std::move(dict));
                           }
                       }

                        json::Print(
                            json::Document{
                                json::Builder{}
                                    .StartDict()
                                        .Key("request_id"s).Value(query.id)
                                        .Key("total_time"s).Value(route_info->route_info->total_time)
                                        .Key("items"s).Value(std::move(arr))
                                    .EndDict()
                                .Build()
                            }, os
                        );
                    }
                    // else incorrect type (never)
                }
            }
            void ToOstream(deque<unique_ptr<OutQuery>>& res, std::ostream& os) {
                os << "["sv;

                for (auto it = res.begin(); it != res.end(); ++it) {
                    if (it == res.begin()) {
                        PrintAnswer(*it->get(), os);
                        continue;
                    }
                    os << ","sv; 
                    PrintAnswer(*it->get(), os);
                }
                os << "]"sv;
            }
        }
        void ProceedProcessQueries(const TransportSystem& system, const Array& queries, 
                    const MapRendererSettings& map_settings, const RouterSetting& router_settings,
                    std::ostream& os = std::cout)
        {
            // Here is two section: processing and interaction with output (cout as default)
            auto answers = process::ProceedAfterInitQueries(system, queries, map_settings, router_settings);

            // Print
            print::ToOstream(answers, os);
        }
    
    }
 
    void ReadJsonFromCin(TransportSystem& system)
    {
        Document doc = Load(cin);

        const auto& root = doc.GetRoot().AsDict();
        const auto& init_queries = root.at("base_requests"s).AsArray();
        const auto& proc_queries = root.at("stat_requests"s).AsArray();
        
        const auto& render_setting = root.at("render_settings"s).AsDict();
        MapRendererSettings map_render_settings = render::InitMapRendererSettings(render_setting);

        const auto& routing_settings = root.at("routing_settings").AsDict();
        RouterSetting router_settings;
        router_settings.SetSettings(
            routing_settings.at("bus_wait_time").AsInt(),
            routing_settings.at("bus_velocity").AsInt()
        );

        detail::ProceedInitQueries(system, init_queries, router_settings);
        detail::ProceedProcessQueries(system, proc_queries, map_render_settings, router_settings, std::cout);
    }
}