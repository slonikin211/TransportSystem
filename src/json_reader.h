#pragma once

#include "json.h"
#include "request_handler.h"

namespace json::read
{
    void ReadJsonFromCin(transport_system::TransportSystem& system);
}