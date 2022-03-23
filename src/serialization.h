#pragma once

#include "transport_catalogue.h"
#include "map_renderer.h"

#include <fstream>
#include <filesystem>
#include <transport_catalogue.pb.h>

namespace serialize
{

class Serializer
{
public:
    Serializer(transport::TransportCatalogue& transport_catalogue, renderer::MapRenderer& map_renderer)
        : transport_catalogue_(transport_catalogue), map_renderer_(map_renderer) {}

    void Serialize();
    void Deserialize();

    void SetFileName(const std::string& filename);
private:
    void SerializeStop();
    void SerializeBus();
    void SerializeDistance();
    void SerializeRenderSettings();
    transport_catalogue_serialize::Color SerializeColor(const svg::Color& color);

    void DeserializeStop();
    void DeserializeBus();
    void DeserializeDistance();
    void DeserializeRenderSettings();
    svg::Color DeserializeColor(const transport_catalogue_serialize::Color& color_pb);
private:
    transport_catalogue_serialize::TransportCatalogue transport_catalogue_serialize_;
    transport::TransportCatalogue& transport_catalogue_;
    renderer::MapRenderer& map_renderer_;
    std::string filename_;
};

}