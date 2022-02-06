#define INPUT_FROM_CIN_JSON

#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

int main()
{
    using namespace transport_system;

#ifdef INPUT_FROM_CIN_JSON
    TransportSystem system;
    json::read::ReadJsonFromCin(system);
#endif  // INPUT_FROM_CIN_JSON

    return 0;
}