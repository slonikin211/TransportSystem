#pragma once

#include <string>
#include <string_view>
#include <unordered_set>
#include <unordered_map>
#include <deque>
#include <set>

#include "domain.h"


namespace transport_system
{
    namespace detail
    {    
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
        void AddRoute(const obj::Bus& bus);
        void AddStop(const obj::Stop& stop);
        void AddLinkStops(const std::pair<const obj::Stop*, const obj::Stop*>& connection, const double route);

        const obj::Bus* FindRouteByBusName(std::string_view name) const;
        const obj::Stop* FindStopByName(std::string_view name) const;
        double FindConnectionValueByStops(const obj::Stop* stop1, const obj::Stop* stop2) const;

        info::BusInfo GetBusInfoByBus(const obj::Bus* bus) const;
        info::StopInfo GetStopInfoByStop(const obj::Stop* stop) const;

        // Mainly for SVG printing info
        const std::deque<const obj::Bus*>& GetBusesPointers() const;
        const std::deque<const obj::Stop*>& GetStopPointers() const;
        const std::deque<info::StopInfo>& GetStopInfo() const;

    private:
        double ComputeGeoRoute(const obj::Bus* bus) const;
        double ComputeRealRoute(const obj::Bus* bus) const;

    private:
        std::deque<obj::Bus> all_busses_;
        std::deque<const obj::Bus*> p_all_busses_;

        std::deque<obj::Stop> all_stops_;
        std::deque<const obj::Stop*> p_all_stops_;
        std::unordered_map<const obj::Stop*, info::StopInfo> all_stops_info_;

        std::unordered_map<std::pair<const obj::Stop*, const obj::Stop*>, double, detail::ConnectionHasher> all_stops_connections_;
    };

}
