//#define TESTS
#define INPUT_FROM_CIN_JSON

#ifdef TESTS
    #include "tests.h"
#endif

#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"

int main()
{
    using namespace transport_system;

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