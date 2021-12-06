//#define TESTS
//#define TEST_RENDERING
#define INPUT_FROM_CIN_JSON

#ifdef TESTS
    #include "tests.h"
#endif
#ifdef TEST_RENDERING
    #include "test_rendering.hpp"
#endif

#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

int main()
{
    using namespace transport_system;

#ifdef TEST_RENDERING
    Test::TestRendering();
#endif

#ifdef TESTS
    Test::TestTransportSystem();
#else

#ifdef INPUT_FROM_CIN_JSON
    TransportSystem system;
    json::read::ReadJsonFromCin(system);
#endif  // INPUT_FROM_CIN_JSON

#endif  // TESTS

    return 0;
}