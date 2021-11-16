#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <set>

#include "geo.h"


namespace transport_system
{
    namespace detail
    {
        // Stop structure include stop name and coordinates
        struct Stop
        {
            std::string name;
            Coordinates coordinates;
        };

        // Bus structure include bus name and route
        struct Bus
        {
            std::string name;
            std::deque<const Stop*> route;
            bool cyclic_route;
        };


        // Bus info for output
        struct BusInfo
        {
            size_t amount_of_stops;
            size_t amount_of_unique_stops;
            double route_length;
            double curvature;
        };


        // Stop info for output
        struct StopInfo
        {
            std::set<const Bus*> buses;
        };
    
        // Hasher for connection
        class ConnectionHasher
        {
        public:
            template <class T1, class T2>
            size_t operator()(const std::pair<T1, T2>& pair) const
            {
                auto h1 = std::hash<T1>{}(pair.first);
                auto h2 = std::hash<T2>{}(pair.second);

                return h1 + h2 * 37u;
            }
        private:
            std::hash<void*> c_hasher_;
        };
    
    }

    // Transport system class which includes next functionality
    // 1. Add route to DB
    // 2. Add stop to DB
    // 3. Find route by name
    // 4. Find stop by name
    // 5. Get information about route

    class TransportSystem
    {
    public:
        void AddRoute(const detail::Bus& bus);
        void AddStop(const detail::Stop& stop);
        void AddLinkStops(const std::pair<const detail::Stop*, const detail::Stop*>& connection, const double route);

        const detail::Bus* FindRouteByBusName(std::string_view name) const;
        const detail::Stop* FindStopByName(std::string_view name) const;
        double FindConnectionValueByStops(const detail::Stop* stop1, const detail::Stop* stop2) const;

        detail::BusInfo GetBusInfoByBus(const detail::Bus* bus) const;
        detail::StopInfo GetStopInfoByStop(const detail::Stop* stop) const;

    private:
        double ComputeGeoRoute(const detail::Bus* bus) const;
        double ComputeRealRoute(const detail::Bus* bus) const;

    private:
<<<<<<< HEAD
        std::unordered_set<detail::Bus*> all_busses_ptrs_;
        std::deque<detail::Bus> all_busses_;

        std::unordered_set<detail::Stop*> all_stops_ptrs_;
=======
        std::deque<detail::Bus> all_busses_;

>>>>>>> master
        std::deque<detail::Stop> all_stops_;
        std::unordered_map<const detail::Stop*, detail::StopInfo> all_stops_info_;

        std::unordered_map<std::pair<const detail::Stop*, const detail::Stop*>, double, detail::ConnectionHasher> all_stops_connections_;
    };

}


namespace help
{
    // Help functionality FOR REMOTE TESTS ONLY (TODO: make another file for this)

    // trim from start (in place)
    void ltrim(std::string_view &s);

    // trim from end (in place)
    void rtrim(std::string_view &s);

    // trim from both ends (in place)
    void trim(std::string_view &s);
}
