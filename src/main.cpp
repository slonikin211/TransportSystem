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
    TransportSystem system;
    InputRead(system);
    InputReadDBQueries(system);
#endif

    return 0;
}