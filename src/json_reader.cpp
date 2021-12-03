#include "json_reader.h"
#include <iostream>
#include <string>
#include <deque>
#include <memory>

using namespace std;
using namespace json;
using namespace transport_system;
using namespace request_handler;
using namespace request_handler::query;
using namespace request_handler::init;
using namespace subjects;
using namespace subjects::geo;
using namespace subjects::obj;
using namespace subjects::info;


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

            deque<unique_ptr<OutQuery>> ProceedAfterInitQueries(const TransportSystem& system, const Array& queries)
            {
                deque<unique_ptr<OutQuery>> result;

                for (const auto& query: queries) {
                    const auto& q = query.AsMap();
                    if (q.at("type"s) == "Bus"s) {
                        result.push_back(move(make_unique<OutBus>(ProceedBusQuery(system, q))));
                    }
                    else if (q.at("type") == "Stop"s) {
                        result.push_back(move(make_unique<OutStop>(ProceedStopQuery(system, q))));
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
                            {"request_id", q.id},
                            {"buses", Array{stop_info->buses.begin(), stop_info->buses.end()}}
                        } };
                        std::visit(json::NodeOutput{os}, answer.GetRootContent());
                    }
                    // else incorrect type (never)
                }
            }
            void ToOstream(deque<unique_ptr<OutQuery>>& res, std::ostream& os = std::cout) {
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
        void ProceedProcessQueries(const TransportSystem& system, const Array& queries)
        {
            // Here is two section: processing and interaction with output (cout as defaul)
            auto answers = process::ProceedAfterInitQueries(system, queries);

            // Print
            print::ToOstream(answers);
        }
    }
 
    void ReadJsonFromCin(TransportSystem& system)
    {
        Document doc = Load(cin);

        const auto& root = doc.GetRoot().AsMap();
        const auto& init_queries = root.at("base_requests").AsArray();
        const auto& proc_queries = root.at("stat_requests").AsArray();

        detail::ProceedInitQueries(system, init_queries);
        detail::ProceedProcessQueries(system, proc_queries);
    }
}