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
        void AddRoute(const subjects::obj::Bus& bus);
        void AddStop(const subjects::obj::Stop& stop);
        void AddLinkStops(const std::pair<const subjects::obj::Stop*, const subjects::obj::Stop*>& connection, const double route);

        const subjects::obj::Bus* FindRouteByBusName(std::string_view name) const;
        const subjects::obj::Stop* FindStopByName(std::string_view name) const;
        double FindConnectionValueByStops(const subjects::obj::Stop* stop1, const subjects::obj::Stop* stop2) const;

        subjects::info::BusInfo GetBusInfoByBus(const subjects::obj::Bus* bus) const;
        subjects::info::StopInfo GetStopInfoByStop(const subjects::obj::Stop* stop) const;

    private:
        double ComputeGeoRoute(const subjects::obj::Bus* bus) const;
        double ComputeRealRoute(const subjects::obj::Bus* bus) const;

    private:
        std::deque<subjects::obj::Bus> all_busses_;

        std::deque<subjects::obj::Stop> all_stops_;
        std::unordered_map<const subjects::obj::Stop*, subjects::info::StopInfo> all_stops_info_;

        std::unordered_map<std::pair<const subjects::obj::Stop*, const subjects::obj::Stop*>, double, detail::ConnectionHasher> all_stops_connections_;
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
