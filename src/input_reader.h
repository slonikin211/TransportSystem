#pragma once

#include <iostream>
#include <unordered_set>
#include <string>

#include "transport_catalogue.h"

namespace transport_system
{
    namespace init
    {
        // Just convert cin to queries
        void InputReadFromCin(transport_system::TransportSystem& t_system);
    }
}

