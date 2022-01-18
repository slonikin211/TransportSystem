#include "map_renderer.h"
#include <deque>
#include <algorithm>
#include <cassert>
#include <fstream>

using namespace svg;
using namespace obj;
using namespace info;
using namespace map_renderer::detail;

template <typename T>
void merge_deques(std::deque<T>& to_, std::deque<T>& from_) {
    for (auto& el: from_) {
        to_.push_back(std::move(el));
    }
}

template <typename T>
void merge_deques(std::deque<T>& to_, std::deque<T>&& from_) {
    for (auto& el: from_) {
        to_.push_back(std::move(el));
    }
}

namespace map_renderer
{
    inline const double EPSILON = 1e-6;
    bool IsZero(double value) {
        return std::abs(value) < EPSILON;
    }

    class SphereProjector
    {
    public:
        template <typename PointInputIt>
        SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding)
            :padding_(padding) 
        {
            if (points_begin == points_end) {
                return;
            }

            const auto [left_it, right_it]
                = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                    return lhs.lng < rhs.lng;
                });
            min_lon_ = left_it->lng;
            const double max_lon = right_it->lng;

            const auto [bottom_it, top_it]
                = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
                    return lhs.lat < rhs.lat;
                });
            const double min_lat = bottom_it->lat;
            max_lat_ = top_it->lat;

            std::optional<double> width_zoom;
            if (!IsZero(max_lon - min_lon_)) {
                width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
            }

            std::optional<double> height_zoom;
            if (!IsZero(max_lat_ - min_lat)) {
                height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
            }

            if (width_zoom && height_zoom) {
                zoom_coeff_ = std::min(*width_zoom, *height_zoom);
            } else if (width_zoom) {
                zoom_coeff_ = *width_zoom;
            } else if (height_zoom) {
                zoom_coeff_ = *height_zoom;
            }
        }

        svg::Point operator()(geo::Coordinates coords) const {
            return {(coords.lng - min_lon_) * zoom_coeff_ + padding_,
                    (max_lat_ - coords.lat) * zoom_coeff_ + padding_};
        }

    private:
        double padding_;
        double min_lon_ = 0;
        double max_lat_ = 0;
        double zoom_coeff_ = 0;
    };
   
    //////////////////////// TransportMap BEGIN ////////////////////////
    
    namespace detail
    {
        size_t TransportMap::Route::current_color = 0u;

        TransportMap::TransportMap(const std::deque<const obj::Bus*>& buses, const std::deque<info::StopInfo>& stops_info,
                const MapRendererSettings& settings, const map_renderer::SphereProjector& projector)
            :buses_(buses), stops_info_(stops_info), settings_(settings), projector_(projector)
        {
        }

        void TransportMap::ProcessDrawing()
        {
            RenderRoutes();
            RenderStops();
        }

        void TransportMap::GetSVGDocument(svg::Document& doc)
        {
            for (auto& el: to_draw_) {
                doc.AddPtr(std::move(el));
            }
        }

        void TransportMap::RenderRoutes()
        {
            std::vector<const Bus*> vec_buses(buses_.begin(), buses_.end());
            // Sort by bus name (use vector instead of deque because of better sort complexety:
            // o(n^2) for vector instead of o(n^3) for deque (o(n) * o(n^2), where o(n) is time for accessing to deque element) 
            std::sort(vec_buses.begin(), vec_buses.end(), [&vec_buses](const Bus* lhs, const Bus* rhs){
                return lhs->name < rhs->name;
            });

            // Proceed drawing
            for (const auto& bus: vec_buses) {
                Route route;
                route.color = settings_.color_palette.at(Route::current_color);
                
                RenderPolyline(route, bus);
                RenderText(route, bus);
                routes_.push_back(route);
                Route::current_color = (Route::current_color + 1u) % settings_.color_palette.size();
            }

            // Add result
            // polylines
            for (auto& route: routes_) {
                to_draw_.push_back(std::make_unique<Polyline>(route.route_line));
            }
            // texts
            for (auto& route: routes_) {
                for (const auto& text: route.route_texts) {
                    to_draw_.push_back(std::make_unique<Text>(text));
                }
            }
        }

        void TransportMap::RenderPolyline(Route& route, const obj::Bus* bus)
        {
            Polyline& line = route.route_line;

            for(const auto stop: bus->route) {
                line.AddPoint(projector_(stop->coordinates));
            }
            if (bus->cyclic_route) { 
                if (bus->route.front()->name != bus->route.back()->name) {
                    line.AddPoint(projector_(bus->route.front()->coordinates));
                }
            }
            else {
                for (auto it = bus->route.rbegin(); it != bus->route.rend(); ++it) {
                    if (it != bus->route.rbegin()) {
                        line.AddPoint(projector_((*it)->coordinates));
                    }
                }
            }
            line
                .SetStrokeColor(route.color)
                .SetFillColor("none"s)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
        }

        void TransportMap::RenderText(Route& route, const obj::Bus* bus)
        {
            // always print first stop
            // if route is not rounded and last stops are not equal then print stop
            // if no stops on the route => skip
            if (bus->route.empty()) {
                return;
            }

            // Finding position of stops
            const Stop* first_stop = bus->route.front();
            const Stop* second_stop = nullptr;
            if (!bus->cyclic_route && bus->route.size() > 1u) {
                if (bus->route.front()->name != bus->route.back()->name) {
                    second_stop = bus->route.back();
                }
            }

            // Add text for first stop
            auto [text, text_under] = GetStopNameText(bus, first_stop, route.color);
            route.route_texts.push_back(text_under);
            route.route_texts.push_back(text);

            // For second stop
            if (second_stop != nullptr) {
                text.SetPosition(projector_(second_stop->coordinates));
                text_under.SetPosition(projector_(second_stop->coordinates));
                route.route_texts.push_back(text_under);
                route.route_texts.push_back(text);
            }
        }

        void TransportMap::RenderStops()
        {
            std::vector<StopInfo> sorted_infos(stops_info_.begin(), stops_info_.end());
            std::sort(sorted_infos.begin(), sorted_infos.end(), 
            [&sorted_infos](const StopInfo& lhs, const StopInfo& rhs) {
                return lhs.stop->name < rhs.stop->name;
            });

            // Proceed
            std::deque<Circle> stops_circles;
            std::deque<Text> stops_names;
            for (const auto& el: sorted_infos) {
                if (!el.buses.empty()) {
                    stops_circles.push_back(RenderStopCirlcle(el.stop));

                    auto [high, under] = RenderStopName(el.stop);
                    stops_names.push_back(under);
                    stops_names.push_back(high);
                }
            }
            
            // Add to to_draw_
            // Circles
            for (auto& circle: stops_circles) {
                to_draw_.push_back(std::make_unique<Circle>(circle));
            }
            // Names
            for (auto& name: stops_names) {
                to_draw_.push_back(std::make_unique<Text>(name));
            }
        }

        Circle TransportMap::RenderStopCirlcle(const obj::Stop* stop)
        {
            Circle circle;
            circle
                .SetCenter(projector_(stop->coordinates))
                .SetRadius(settings_.stop_radius)
                .SetFillColor("white"s);
            return circle;
        }

        std::pair<svg::Text, svg::Text> TransportMap::RenderStopName(const obj::Stop* stop)
        {
            assert(stop != nullptr);

            Text high, under;
            high
                .SetPosition(projector_(stop->coordinates))
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetData(stop->name)
                .SetFillColor("black");

            under
                .SetPosition(projector_(stop->coordinates))
                .SetOffset(settings_.stop_label_offset)
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetData(stop->name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
            return {high, under};
        }

        std::pair<svg::Text, svg::Text> TransportMap::GetStopNameText(const obj::Bus* bus, const obj::Stop* stop, const Color& color)
        {
            assert(stop != nullptr);

            Text high, under;
            high
                .SetPosition(projector_(stop->coordinates))
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetData(bus->name)
                .SetFillColor(color);

            under
                .SetPosition(projector_(stop->coordinates))
                .SetOffset(settings_.bus_label_offset)
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana"s)
                .SetFontWeight("bold"s)
                .SetData(bus->name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(StrokeLineCap::ROUND)
                .SetStrokeLineJoin(StrokeLineJoin::ROUND);
            return {high, under};
        }

    } // namespace detail
    //////////////////////// TransportMap END ////////////////////////

    std::deque<geo::Coordinates> GetPointsFromStops(const std::deque<StopInfo>& stops_info) 
    {
        std::deque<geo::Coordinates> res;
        for (const auto& stop: stops_info) {
            if (!stop.buses.empty()) {
                res.push_back(stop.stop->coordinates);
            }
        }
        return res;
    }
 
    void PrintSVGMap(const std::deque<const obj::Bus*>& buses,
        std::ostream& out, const std::deque<StopInfo>& stops_info,
        const map_renderer::detail::MapRendererSettings& settings)
    {
        using namespace detail;

        auto points = GetPointsFromStops(stops_info);
        map_renderer::SphereProjector projector(points.begin(), points.end(), settings.width, settings.height, settings.padding);

        TransportMap t_map(buses, stops_info, settings, projector);
        t_map.ProcessDrawing();
        
        Document document;
        t_map.GetSVGDocument(document);
        std::ofstream ofs("output.svg"s);
        document.Render(ofs);
        document.Render(out);
    }
} // namespace render