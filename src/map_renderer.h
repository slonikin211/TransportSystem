#pragma once

#include "svg.h"
#include "domain.h"
#include "geo.h"
#include <utility>
#include <vector>
#include <string>
#include <deque>
#include <memory>
#include <iostream>

// For test
using namespace std::string_literals;

namespace map_renderer
{
    class SphereProjector;
    namespace detail
    {
        struct MapRendererSettings
        {
            double width, height, padding, line_width, stop_radius;
            int stop_label_font_size, bus_label_font_size;
            svg::Point stop_label_offset, bus_label_offset;
            
            svg::Color underlayer_color;
            double underlayer_width;
            std::vector<svg::Color> color_palette;
        };

        class TransportMap
        {
        public:
            TransportMap(const std::deque<const subjects::obj::Bus*>& buses, const std::deque<subjects::info::StopInfo>& stops_info,
                const MapRendererSettings& settings, const map_renderer::SphereProjector& projector);
            void ProcessDrawing();
            void GetSVGDocument(svg::Document& doc);

        private:
            struct Route {
                svg::Color color;
                svg::Polyline route_line;
                std::deque<svg::Text> route_texts;
                static size_t current_color;
            };
            std::deque<Route> routes_;
            std::deque<std::unique_ptr<svg::Object>> to_draw_;

        private:
            // Render section. Just adds Objects to do_draw_
            void RenderRoutes();
            void RenderStops();

            void RenderPolyline(Route& route, const subjects::obj::Bus* bus);
            void RenderText(Route& route, const subjects::obj::Bus* bus);
            svg::Circle RenderStopCirlcle(const subjects::obj::Stop* stop);
            std::pair<svg::Text, svg::Text> RenderStopName(const subjects::obj::Stop* stop);
        
            // Help functions
            std::pair<svg::Text, svg::Text> GetStopNameText(const subjects::obj::Bus* bus, const subjects::obj::Stop* stop, const svg::Color& color);  
        private:
            const std::deque<const subjects::obj::Bus*>& buses_;
            const std::deque<subjects::info::StopInfo>& stops_info_;
            const MapRendererSettings& settings_;
            const map_renderer::SphereProjector& projector_;
        };
    }

    void PrintSVGMap(const std::deque<const subjects::obj::Bus*>& buses,
        std::ostream& out, const std::deque<subjects::info::StopInfo>& stops_info,
        const map_renderer::detail::MapRendererSettings& settings);
} // map_renderer
