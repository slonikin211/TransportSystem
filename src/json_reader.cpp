#include "json_reader.h"
#include "map_renderer.h"
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
using namespace request_handler::query;
using namespace request_handler::init;
using namespace subjects;
using namespace subjects::geo;
using namespace subjects::obj;
using namespace subjects::info;
using namespace map_renderer;
using namespace map_renderer::detail;


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

    MapRendererSettings InitSettings(const Dict& root) 
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
                query::InitStop MakeQueryInitStop(const Dict& q)
                {
                    const string& type = q.at("type"s).AsString();

                    if (type != "Stop"s) {
                        throw logic_error("Invalid type for MakeQueryInitStop");
                    }

                    const string& name = q.at("name"s).AsString();
                    double lat = q.at("latitude"s).AsDouble();
                    double lng = q.at("longitude"s).AsDouble();

                    const Dict& road_distances = q.at("road_distances"s).AsMap();
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
            
                query::InitBus MakeQueryInitBus(const Dict& q)
                {
                    const string& type = q.at("type"s).AsString();

                    if (type != "Bus"s) {
                        throw logic_error("Invalid type for MakeQueryInitBus");
                    }

                    const string& name = q.at("name"s).AsString();
                    bool is_roundtrip = q.at("is_roundtrip"s).AsBool();

                    const Array& stops = q.at("stops"s).AsArray();
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

            void ProceedInitStop(TransportSystem& system, const Dict& q)
            {
                const string& type = q.at("type"s).AsString();
                if (type != "Stop"s) {
                    return;
                }
                request_handler::init::InitStop(system, MakeQueryInitStop(q));
            }

            void ProceedInitQuery(TransportSystem& system, const Dict& q)
            {
                const string& type = q.at("type"s).AsString();
                if (type == "Stop"s) {
                    // Init was at previos finction called ProceedInitStop, so here is just linking
                    request_handler::init::InitStopsConnections(system, MakeQueryInitStop(q));                  
                }
                else if (type == "Bus"s) {
                    request_handler::init::InitBus(system, MakeQueryInitBus(q));
                }
            }

        }
        void ProceedInitQueries(TransportSystem& system, const Array& init)
        {
            // Init stops first
            for (const auto& query: init) {
                init::ProceedInitStop(system, query.AsMap());
            }

            // Init all queries (with link stops)
            for (const auto& query: init) {
                init::ProceedInitQuery(system, query.AsMap());
            }
        }
    

        namespace process
        {
            // TODO: Proceed this function (getting document and render it to ostream. ostream to str for saving result)
           // and integrate with map_renderer 
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

            deque<unique_ptr<OutQuery>> ProceedAfterInitQueries(const TransportSystem& system, const Array& queries, const MapRendererSettings& settings)
            {
                deque<unique_ptr<OutQuery>> result;

                for (const auto& query: queries) {
                    const auto& q = query.AsMap();
                    if (q.at("type"s) == "Bus"s) {
                        result.push_back(move(make_unique<OutBus>(ProceedBusQuery(system, q))));
                    }
                    else if (q.at("type"s) == "Stop"s) {
                        result.push_back(move(make_unique<OutStop>(ProceedStopQuery(system, q))));
                    }
                    else if (q.at("type"s) == "Map"s) {
                        result.push_back(move(make_unique<OutMap>(ProceedRenderQuery(system, q, settings))));
                    }
                }

                return result;
            }
        }

        namespace print
        {
            namespace
            {
                void PrintAnswer(request_handler::query::OutQuery& q, std::ostream& os)
                {
                    if (q.additional_data == "not found"s) {
                        Node answer{Dict{
                            {"request_id"s, q.id},
                            {"error_message"s, "not found"s}
                        }};
                        std::visit(json::NodeOutput{os}, answer.GetRootContent());
                        return;
                    }

                    if (const OutBus* bus_info = dynamic_cast<const OutBus*>(&q)) {
                        Node answer{Dict{
                            {"request_id"s, q.id},
                            {"curvature"s, bus_info->curvature},
                            {"route_length"s, bus_info->route_length},
                            {"stop_count"s, bus_info->stop_count},
                            {"unique_stop_count"s, bus_info->unique_stop_count}
                        } };
                        std::visit(json::NodeOutput{os}, answer.GetRootContent());
                    }
                    else if (const OutStop* stop_info = dynamic_cast<const OutStop*>(&q)) {
                        Node answer{Dict{
                            {"request_id"s, q.id},
                            {"buses"s, Array{stop_info->buses.begin(), stop_info->buses.end()}}
                        } };
                        std::visit(json::NodeOutput{os}, answer.GetRootContent());
                    }
                    else if (const OutMap* map_info = dynamic_cast<const OutMap*>(&q)) {
                        // TODO: Разобраться с \n для вывода
                        std::string data = map_info->os.str();
                        Node answer{Dict{
                            {"request_id"s, q.id},
                            {"map"s, Node(data)}
                        } };
                        std::visit(json::NodeOutput{os}, answer.GetRootContent());
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
        void ProceedProcessQueries(const TransportSystem& system, const Array& queries, const MapRendererSettings& settings, std::ostream& os = std::cout)
        {
            // Here is two section: processing and interaction with output (cout as defaul)
            auto answers = process::ProceedAfterInitQueries(system, queries, settings);

            // Print
            print::ToOstream(answers, os);
        }
    
    }
 
    void ReadJsonFromCin(TransportSystem& system)
    {
        Document doc = Load(cin);

        const auto& root = doc.GetRoot().AsMap();
        const auto& init_queries = root.at("base_requests"s).AsArray();
        const auto& proc_queries = root.at("stat_requests"s).AsArray();
        
        const auto& render_setting = root.at("render_settings"s).AsMap();
        MapRendererSettings settings = render::InitSettings(render_setting);

        // For test
        //std::ofstream ofs("../test_output/TestSVGFromCin.svg"s);

        detail::ProceedInitQueries(system, init_queries);
        detail::ProceedProcessQueries(system, proc_queries, settings, std::cout);
    }
}