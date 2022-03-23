#pragma once

#include "svg.h"
#include "domain.h"
#include "geo.h"

#include <vector>
#include <memory>
#include <algorithm>
#include <optional>
#include <cmath>
#include <utility>
#include <cstdlib>

namespace renderer 
{
	class SphereProjector 
    {
	public:
		template <typename PointInputIt>
		SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width, double max_height, double padding) : padding_(padding) 
        {
			if (points_begin == points_end) { return; }

			const auto [left_it, right_it] = std::minmax_element(
				points_begin,
				points_end,
				[](auto lhs, auto rhs) {
					return lhs.lng < rhs.lng;
				}
			);
			min_lon_ = left_it->lng;
			const double max_lon = right_it->lng;

			const auto [bottom_it, top_it] = std::minmax_element(
				points_begin,
				points_end,
				[](auto lhs, auto rhs) {
					return lhs.lat < rhs.lat;
				}
			);
			const double min_lat = bottom_it->lat;
			max_lat_ = top_it->lat;

			std::optional<double> width_zoom;
			if (!IsZero(max_lon - min_lon_)) 
            {
				width_zoom = (max_width - 2.0 * padding) / (max_lon - min_lon_);
			}

			std::optional<double> height_zoom;
			if (!IsZero(max_lat_ - min_lat)) 
            {
				height_zoom = (max_height - 2.0 * padding) / (max_lat_ - min_lat);
			}

			if (width_zoom && height_zoom) 
            {
				zoom_coeff_ = std::min(*width_zoom, *height_zoom);
			}
			else if (width_zoom) 
            {
				zoom_coeff_ = *width_zoom;
			}
			else if (height_zoom) 
            {
				zoom_coeff_ = *height_zoom;
			}
		}

		svg::Point operator()(geo::Coordinates coords) const;

	private:
		double padding_ = 0.0;
		double min_lon_ = 0.0;
		double max_lat_ = 0.0;
		double zoom_coeff_ = 0.0;

		constexpr static const double EPSILON = 1e-6;
		static bool IsZero(double value)
        {
			return std::abs(value) < EPSILON;
		}
	};

	struct RenderingSettings {
		double width = 0;
		double height = 0;
		double padding = 0;
		double line_width = 0;
		double stop_radius = 0;

		int bus_label_font_size = 0;
		svg::Point bus_label_offset;

		int stop_label_font_size = 0;
		svg::Point stop_label_offset;

		svg::Color underlayer_color;
		double underlayer_width = 0;

		std::vector<svg::Color> color_palette;
	};

	class MapRenderer {
	public:
		MapRenderer() = default;
		MapRenderer(RenderingSettings&& settings);

		void SetSettings(RenderingSettings&& settings);
		svg::Document MakeDocument(std::vector<domain::BusPointer>&& buses, std::vector<std::pair<domain::StopPointer, domain::StopInfo>>&& stops) const;

		const RenderingSettings& GetRenderSettings() const;
	private:
		RenderingSettings settings_;

		template <typename It>
		std::vector<geo::Coordinates> StopsToCoordinates(It begin, It end) const 
        {
			std::vector<geo::Coordinates> result;
			result.reserve(end - begin);
			for (It it = begin; it != end; ++it) 
            {
				if (it->second.passing_buses != nullptr && !it->second.passing_buses->empty()) 
                {
					result.emplace_back(geo::Coordinates{ it->first.get()->coords.lat, it->first.get()->coords.lng });
				}
			}
			return result;
		}

		void AddBusesLines(svg::Document& doc, SphereProjector& proj, const std::vector<domain::BusPointer>& buses) const;
		void AddBusesNames(svg::Document& doc, SphereProjector& proj, const std::vector<domain::BusPointer>& buses) const;
		void AddStopsCircles(svg::Document& doc, SphereProjector& proj, const std::vector<std::pair<domain::StopPointer, domain::StopInfo>>& stops) const;
		void AddStopsNames(svg::Document& doc, SphereProjector& proj, const std::vector<std::pair<domain::StopPointer, domain::StopInfo>>& stops) const;
	};
}
