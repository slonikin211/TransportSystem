#include "request_handler.h"

#include <map>
#include <iostream>
#include <unordered_set>
#include <string_view>
#include <string>
#include <deque>
#include <sstream>
#include <stdexcept>
#include <ostream>

using namespace std::string_literals;

using namespace transport_system;
using namespace transport_system::detail;
using namespace obj;
using namespace info;
using namespace svg;
using namespace map_renderer;
using namespace map_renderer::detail;

namespace request_handler
{
    namespace init
    {

        // HERE IS PROBLEM FUNCTION
        // Неправильно считает время (в 1 примере у запросов ошибка у 297 автобуса)

        void FillTransportRouter(const transport_system::TransportSystem& system, transport_system::RouterSetting& setting)
        {
            for (const auto stop: system.GetStopPointers()) 
            {
                const std::string_view stop_name(stop->name);
                setting.AddStop(stop_name);
                setting.AddWaitEdge(stop_name);
            }

            for (const auto bus: system.GetBusesPointers())
            {
                const std::string_view bus_name(bus->name);
                
                int i = 0;
                for (const auto stop_from: bus->route)
                {
                    if (i == bus->route.size() - 1) { break; }  // instead of last + 1

                    const std::string_view stop_name_from = stop_from->name;
                    
                    double prev_actual = 0.0;
                    std::string_view prev_stop_name = stop_name_from;

                    int j = 0;
                    for (const auto stop_to: bus->route)
                    {
                        // костыль но можно все остановки в вектор запихнуть, но тут затраты на копирование указателей
                        if (j < i + 1) {
                            ++j;
                            continue;
                        }

                        const std::string_view stop_name_to = stop_to->name;
                        double actual = system.FindConnectionValueByStops(stop_from, stop_to);

                        if (std::fabs(actual) > 0.0001)    // counting only existing links
                        {
                            setting.AddBusEdge(
                                stop_name_from,
                                stop_name_to,
                                bus_name,
                                j - i,  // stop count between stops
                                prev_actual + actual
                            );
                            prev_stop_name = stop_name_to;
                            prev_actual += actual;
                        }
                        ++j;
                    }
                    ++i;
                }
            }
            setting.BuildGraph();
            setting.BuildRouter();
        }

        void InitStopsConnections(TransportSystem& system, const request_handler::query::InitStop& query)
        {
            const Stop* first = system.FindStopByName(query.name);
            const Stop* second;

            for (const auto& stop: query.road_distances) {
                second = system.FindStopByName(stop.first); // find by name (.first)
                system.AddLinkStops({first, second}, stop.second);   // .second is the length of route
            }
        }

        void InitStop(TransportSystem& system, const request_handler::query::InitStop& query)
        {
            system.AddStop({
                query.name,
                query.coords
            });
        }

        void InitBus(TransportSystem& system, const request_handler::query::InitBus& query)
        {
            std::deque<const Stop*> route;
            for (const auto& stop: query.stops) {
                route.push_back(system.FindStopByName(stop));
            }

            system.AddRoute({
                query.name,
                std::move(route),
                query.is_roundtrip
            });
        }

    } // init

    namespace process
    {
        BusInfo GetBusInfo(const transport_system::TransportSystem& system, const request_handler::query::GetInfo& info)
        {
            if (info.type != "Bus"s) {
                throw std::logic_error("This is not a bus (GetBusInfo)");
            }
            return system.GetBusInfoByBus(system.FindRouteByBusName(info.name));
        }

        StopInfo GetStopInfo(const transport_system::TransportSystem& system, const request_handler::query::GetInfo& info)
        {
            if (info.type != "Stop"s) {
                throw std::logic_error("This is not a stop (GetStopInfo)");
            }
            return system.GetStopInfoByStop(system.FindStopByName(info.name));
        }

        void GetSVGDocument(const transport_system::TransportSystem& system, const map_renderer::detail::MapRendererSettings& settings, std::ostream& os)
        {
            PrintSVGMap(system.GetBusesPointers(), os, system.GetStopInfo(), settings);
        }

        std::optional<transport_system::OutRouteinfo> GetRouteInfo(const transport_system::RouterSetting& setting,
            const std::string_view from, const std::string_view to)
        {
            return setting.GetRouteInfo(from, to);
        }
    } // process
} // request_handler