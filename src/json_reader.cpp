#include "json_reader.h"
#include "transport_router.h"

#include <utility>
#include <unordered_set>
#include <set>
#include <algorithm>
#include <sstream>
#include <cassert>

namespace json_reader 
{
	using namespace domain;
	using namespace std::literals;

	JsonReader::JsonReader(request_handler::RequestHandler& req_handler) : rh_(req_handler) {}

	void JsonReader::Start(std::istream& input, std::ostream& out) 
    {
		const json::Document doc = json::Load(input);
		const json::Node& node = doc.GetRoot();
		const json::Dict& dict = node.AsDict();

		if (dict.count("routing_settings"s)) 
        {
			const auto [wait, vel] = ReadRoutingSettings(dict.at("routing_settings"s).AsDict());
			rh_.SetRoutingSettings(wait, vel);
		}
		if (dict.count("base_requests"s)) 
        {
			FillTransportCatalogue(dict);
			FillGraphInRouter();
		}
		if (dict.count("render_settings"s)) 
        {
			rh_.SetRenderSettings(std::move(ReadRenderingSettings(dict)));
		}
		if (dict.count("stat_requests"s)) 
        {
			AnswerStatRequests(dict, out);
		}
	}

	void JsonReader::MakeBase(std::istream& input)
	{
		const json::Document doc = json::Load(input);
		const json::Node& node = doc.GetRoot();
		const json::Dict& dict = node.AsDict();

		if (dict.count("routing_settings"s)) 
        {
			const auto [wait, vel] = ReadRoutingSettings(dict.at("routing_settings"s).AsDict());
			rh_.SetRoutingSettings(wait, vel);
		}
		if (dict.count("base_requests"s)) 
        {
			FillTransportCatalogue(dict);
			FillGraphInRouter();
		}
		if (dict.count("render_settings"s)) 
        {
			rh_.SetRenderSettings(std::move(ReadRenderingSettings(dict)));
		}
		if (dict.count("serialization_settings"s)) 
        {
			rh_.SetSerializationSettings(dict.at("serialization_settings"s).AsDict().at("file").AsString());
			rh_.Serialize();
		}
	}

	void JsonReader::ProcessRequests(std::istream& input, std::ostream& out)
	{
		const json::Document doc = json::Load(input);
		const json::Node& node = doc.GetRoot();
		const json::Dict& dict = node.AsDict();

		if (dict.count("serialization_settings"s)) 
        {
			rh_.SetSerializationSettings(dict.at("serialization_settings"s).AsDict().at("file").AsString());
			rh_.Deserialize();
		}
		if (dict.count("stat_requests"s)) 
        {
			AnswerStatRequests(dict, out);
		}
	}

	void JsonReader::FillTransportCatalogue(const json::Dict& dict) 
    {
		const json::Array& base_requests = dict.at("base_requests"s).AsArray();
		std::vector<std::pair<const std::string&, const json::Dict&>> stops_road_distances;

		for (const auto& req_node : base_requests) 
        {
			const json::Dict& req = req_node.AsDict();
			if (req.at("type"s).AsString() == "Stop"s) 
            {
				stops_road_distances.emplace_back(req.at("name"s).AsString(), FillStop(req));
			}
		}

		for (const auto& [stop_name, dict] : stops_road_distances) 
        {
			for (const auto& [stop_name_to, distance] : dict) 
            {
				rh_.SetDistanceBetweenStops(stop_name, stop_name_to, distance.AsInt());
			}
		}

		for (const auto& req_node : base_requests) 
        {
			const json::Dict& req = req_node.AsDict();
			if (req.at("type"s).AsString() == "Bus"s) 
            {
				FillBus(req);
			}
		}
	}

	void JsonReader::FillGraphInRouter() 
    {
		rh_.FillRouter();
	}

	const json::Dict& JsonReader::FillStop(const json::Dict& stop_req) 
    {
		const auto& node_latitude = stop_req.at("latitude"s);
		double latitude = node_latitude.IsPureDouble() ? node_latitude.AsDouble() : node_latitude.AsInt();
		const auto& node_longitude = stop_req.at("longitude"s);
		double longitude = node_longitude.IsPureDouble() ? node_longitude.AsDouble() : node_longitude.AsInt();
		Stop stop(std::move(std::string(stop_req.at("name"s).AsString())), latitude, longitude);
		rh_.AddStop(std::move(stop));

		return stop_req.at("road_distances"s).AsDict();
	}

	void JsonReader::FillBus(const json::Dict& bus_req) 
	{
		auto [route, unique_stops_num, last_stop] = WordsToRoute(bus_req.at("stops"s).AsArray(), bus_req.at("is_roundtrip"s).AsBool());
		const auto [geographic, actual] = rh_.ComputeRouteLengths(route);
		if (last_stop.get() == rh_.FindStop(route.front()).get()) 
        {
			Bus bus(std::move(std::string(bus_req.at("name"s).AsString())), rh_.StopsToStopPointer(std::move(route)), unique_stops_num, actual, geographic);
			rh_.AddBus(std::move(bus));
		}
		else 
        {
			Bus bus(std::move(std::string(bus_req.at("name"s).AsString())), rh_.StopsToStopPointer(std::move(route)), unique_stops_num, actual, geographic, last_stop);
			rh_.AddBus(std::move(bus));
		}
	}

	std::tuple<json_reader::JsonReader::BusWaitTime, json_reader::JsonReader::BusVelocity> JsonReader::ReadRoutingSettings(const json::Dict& dict) 
    {
		const int wait_time = dict.at("bus_wait_time"s).AsInt();
		const double velocity = GetDoubleFromNode(dict.at("bus_velocity"s));
		return { wait_time, velocity };
	}

	renderer::RenderingSettings JsonReader::ReadRenderingSettings(const json::Dict& dict) 
    {
		renderer::RenderingSettings settings;

		const json::Dict& dict_deeper = dict.at("render_settings"s).AsDict();

		const json::Node& node_width = dict_deeper.at("width"s);
		settings.width = GetDoubleFromNode(node_width);

		const json::Node& node_height = dict_deeper.at("height"s);
		settings.height = GetDoubleFromNode(node_height);

		const json::Node& node_padding = dict_deeper.at("padding"s);
		settings.padding = GetDoubleFromNode(node_padding);

		const json::Node& node_stop_radius = dict_deeper.at("stop_radius"s);
		settings.stop_radius = GetDoubleFromNode(node_stop_radius);

		const json::Node& node_line_width = dict_deeper.at("line_width"s);
		settings.line_width = GetDoubleFromNode(node_line_width);

		settings.bus_label_font_size = dict_deeper.at("bus_label_font_size"s).AsInt();

		const json::Array& arr_bus_label_offset = dict_deeper.at("bus_label_offset"s).AsArray();
		settings.bus_label_offset.x = GetDoubleFromNode(arr_bus_label_offset[0u]);
		settings.bus_label_offset.y = GetDoubleFromNode(arr_bus_label_offset[1u]);

		settings.stop_label_font_size = dict_deeper.at("stop_label_font_size"s).AsInt();

		const json::Array& arr_stop_label_offset = dict_deeper.at("stop_label_offset"s).AsArray();
		settings.stop_label_offset.x = GetDoubleFromNode(arr_stop_label_offset[0u]);
		settings.stop_label_offset.y = GetDoubleFromNode(arr_stop_label_offset[1u]);

		const json::Node& arr_underlayer_color = dict_deeper.at("underlayer_color"s);
		settings.underlayer_color = GetColor(arr_underlayer_color);

		const json::Node& node_underlayer_width = dict_deeper.at("underlayer_width"s);
		settings.underlayer_width = GetDoubleFromNode(node_underlayer_width);

		const json::Array& node_color_palette = dict_deeper.at("color_palette"s).AsArray();
		settings.color_palette = GetColorsFromArray(node_color_palette);

		return settings;
	}

	double JsonReader::GetDoubleFromNode(const json::Node& node) const 
    {
		return (node.IsPureDouble() ? node.AsDouble() : node.AsInt());
	}

	std::vector<svg::Color> JsonReader::GetColorsFromArray(const json::Array& arr) const 
    {
		std::vector<svg::Color> result;
		result.reserve(arr.size());

		for (size_t i = 0u; i < arr.size(); ++i) 
        {
			const json::Node& node = arr[i];
			result.emplace_back(std::move(GetColor(node)));
		}

		return result;
	}

	svg::Color JsonReader::GetColor(const json::Node& node) const 
	{
		if (node.IsString()) 
		{
			return node.AsString();
		}
		else if (node.IsArray()) {
			const json::Array& arr = node.AsArray();
			if (arr.size() == 3u) 
			{
				return
					svg::Rgb{
						static_cast<uint8_t>(arr[0u].AsInt()),
						static_cast<uint8_t>(arr[1u].AsInt()),
						static_cast<uint8_t>(arr[2u].AsInt())
				};
			}
			else {
				return
					svg::Rgba{
						static_cast<uint8_t>(arr[0u].AsInt()),
						static_cast<uint8_t>(arr[1u].AsInt()),
						static_cast<uint8_t>(arr[2u].AsInt()),
						GetDoubleFromNode(arr[3u])
				};
			}
		}

		return {};
	}

	void JsonReader::AnswerStatRequests(const json::Dict& dict, std::ostream& out) const {
		json::Array result;
		const json::Array& stat_requests = dict.at("stat_requests"s).AsArray();
		for (const auto& req_node : stat_requests)
		{
			const json::Dict& req = req_node.AsDict();
			const std::string& type = req.at("type"s).AsString();
			json::Node node;
			if (type == "Stop"s)
			{
				node = OutStopStat(
					rh_.GetStopInfo(req.at("name"s).AsString()),
					req.at("id"s).AsInt()
				);
			}
			else if (type == "Bus"s) 
			{
				node = OutBusStat(
					rh_.GetBusInfo(req.at("name"s).AsString()),
					req.at("id"s).AsInt()
				);
			}
			else if (type == "Route"s)
			{
				node = OutRouteReq(
					req.at("from"s).AsString(),
					req.at("to"s).AsString(),
					req.at("id"s).AsInt()
				);
			}
			else 
			{
				node = OutMapReq(req.at("id"s).AsInt());
			}
			result.push_back(std::move(node));
		}

		json::Print(json::Document(json::Node(result)), out);
	}

	json::Node JsonReader::OutStopStat(const std::optional<StopInfo> stop_stat, int id) const 
	{
		if (stop_stat.has_value()) 
		{
			json::Array arr;
			if (stop_stat->passing_buses == nullptr) 
			{
				json::Dict dict = {
					{ "buses"s,      json::Node(std::move(arr)) },
					{ "request_id"s, json::Node(id)             }
				};

				return json::Node(std::move(dict));
			}
			const auto buses = *stop_stat->passing_buses;
			arr.reserve(buses.size());
			std::vector<BusPointer> tmp(buses.begin(), buses.end());
			std::sort(tmp.begin(), tmp.end(),
				[](const BusPointer& lhs, const BusPointer& rhs) 
				{
					return std::lexicographical_compare(
						lhs.get()->name.get()->begin(), lhs.get()->name.get()->end(),
						rhs.get()->name.get()->begin(), rhs.get()->name.get()->end()
					);
				}
			);
			for (const BusPointer& bus : std::move(tmp)) 
			{
				arr.push_back(json::Node(*bus.get()->name.get()));
			}
			json::Dict dict = {
				{ "buses"s,      json::Node(std::move(arr)) },
				{ "request_id"s, json::Node(id)             }
			};

			return json::Node(std::move(dict));
		}
		else {
			json::Dict dict = {
				{ "request_id"s,    json::Node(id)                      },
				{ "error_message"s, json::Node(std::move("not found"s)) }
			};

			return json::Node(std::move(dict));
		}
	}

	json::Node JsonReader::OutBusStat(const std::optional<BusInfo> bus_stat, int id) const 
	{
		if (bus_stat.has_value()) 
		{
			json::Dict dict = {
				{ "request_id"s,        json::Node(id)                            },
				{ "curvature"s,         json::Node(bus_stat->curvature)           },
				{ "unique_stop_count"s, json::Node(bus_stat->unique_stops)        },
				{ "stop_count"s,        json::Node(bus_stat->stops_on_route)      },
				{ "route_length"s,      json::Node(bus_stat->routh_actual_length) }
			};

			return json::Node(std::move(dict));
		}
		else 
		{
			json::Dict dict = {
				{ "request_id"s,    json::Node(id)                      },
				{ "error_message"s, json::Node(std::move("not found"s)) }
			};

			return json::Node(std::move(dict));
		}
	}

	json::Node JsonReader::OutRouteReq(const std::string_view from, const std::string_view to, int id) const 
	{
		const auto route_info = rh_.GetRouteInfo(from, to);
		if (route_info) {
			json::Array arr;
			arr.reserve(route_info->items.size());
			for (const auto item : route_info->items) 
			{
				if (item.wait_item) 
				{
					std::string stop_name(item.wait_item->stop_name);
					json::Dict dict = {
						{ "type"s,      json::Node(std::move("Wait"s))   },
						{ "stop_name"s, json::Node(std::move(stop_name)) },
						{ "time"s,      json::Node(item.wait_item->time) }
					};
					arr.push_back(std::move(dict));
				}
				else 
				{
					std::string bus_name(item.bus_item->bus_name);
					json::Dict dict = {
						{ "type"s,       json::Node(std::move("Bus"s))         },
						{ "bus"s,        json::Node(std::move(bus_name))       },
						{ "span_count"s, json::Node(item.bus_item->span_count) },
						{ "time"s,       json::Node(item.bus_item->time)       }
					};
					arr.push_back(std::move(dict));
				}
			}

			json::Dict dict = {
				{ "request_id"s, json::Node(id)                     },
				{ "total_time"s, json::Node(route_info->total_time) },
				{ "items"s,      json::Node(std::move(arr))         }
			};

			return json::Node(std::move(dict));
		}
		else 
		{
			json::Dict dict = {
				{ "request_id"s,    json::Node(id)                      },
				{ "error_message"s, json::Node(std::move("not found"s)) }
			};

			return json::Node(std::move(dict));
		}
	}

	json::Node JsonReader::OutMapReq(int id) const 
	{
		std::ostringstream out;
		svg::Document doc = rh_.RenderMap();
		doc.Render(out);

		json::Dict dict = {
			{ "request_id"s, json::Node(id) },
			{ "map"s, json::Node(std::move(out.str())) }
		};

		return json::Node(std::move(dict));
	}

	std::tuple<std::vector<std::string_view>, int, StopPointer> JsonReader::WordsToRoute(const json::Array& words, bool is_roundtrip) const {
		std::vector<std::string_view> result;
		std::unordered_set<std::string_view, std::hash<std::string_view>> stops_unique_names;
		result.reserve(words.size());

		for (size_t i = 0u; i < words.size(); ++i) 
		{
			StopPointer stop = rh_.FindStop(words[i].AsString());
			result.push_back(*stop.get()->name.get());
			stops_unique_names.insert(words[i].AsString());
		}
		const StopPointer last_stop = rh_.FindStop(result.back());

		if (!is_roundtrip && words.size() > 1u) // Go back if roundtrip
		{
			result.reserve(words.size() * 2u);
			for (int i = (int)words.size() - 2; i >= 0; --i) 
			{
				StopPointer stop = rh_.FindStop(words[i].AsString());
				result.push_back(*stop.get()->name.get());
			}
		}

		return {
			std::move(result),
			static_cast<int>(stops_unique_names.size()),
			last_stop
		};
	}
}
