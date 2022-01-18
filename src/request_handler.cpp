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
    } // process
} // request_handler