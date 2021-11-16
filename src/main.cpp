//#define TESTS

#ifdef TESTS
    #include "tests.h"
#endif

#include "transport_catalogue.h"
#include "input_reader.h"
#include "stat_reader.h"

int main()
{
#ifdef TESTS
    Test::TestTransportSystem();
#else
    using namespace transport_system;
    using namespace transport_system::init;
    using namespace transport_system::read_queries;

    TransportSystem system;
    InputReadFromCin(system);
    InputReadDBQueriesFromCin(system);
#endif

    return 0;
}