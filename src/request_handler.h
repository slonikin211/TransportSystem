#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"
#include <vector>
#include <map>
#include <sstream>

namespace request_handler
{
    namespace query
    {
        ////////////////////////// Init //////////////////////////

        struct InitStop
        {
            std::string name;
            geo::Coordinates coords;
            std::map<std::string, double> road_distances;
        };
        struct InitBus
        {
            std::string name;
            std::vector<std::string> stops;
            bool is_roundtrip;
        };
    
        ////////////////////////// Process //////////////////////////
        
        struct GetInfo
        {
            int id;
            std::string type, name;
        };

        ////////////////////////// Result (output) //////////////////////////

        struct OutQuery
        {
            int id;
            std::string additional_data;   // if bus/stop is not found or something else as an example for usage
            virtual ~OutQuery() = default;
        };
        
        struct OutBus : OutQuery
        {
            double curvature;
            double route_length;
            int stop_count, unique_stop_count;
        };

        struct OutStop : OutQuery
        {
            std::set<std::string> buses;
        };

        struct OutMap : OutQuery
        {
            std::ostringstream os;
        };
    } // query

    namespace init
    {
        void InitStopsConnections(transport_system::TransportSystem& system, const query::InitStop& query);
        void InitStop(transport_system::TransportSystem& system, const query::InitStop& query);
        void InitBus(transport_system::TransportSystem& system, const query::InitBus& query);
    } // init

    namespace process
    {
        info::BusInfo GetBusInfo(const transport_system::TransportSystem& system, const request_handler::query::GetInfo& info);
        info::StopInfo GetStopInfo(const transport_system::TransportSystem& system, const request_handler::query::GetInfo& info);
        void GetSVGDocument(const transport_system::TransportSystem& system, const map_renderer::detail::MapRendererSettings& settings, std::ostream& os);
    } // process
} // request_handler