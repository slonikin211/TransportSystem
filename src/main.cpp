#define TESTS

#include "tests.h"

int main()
{
#ifdef TESTS
    Test::TestTransportSystem();
#endif

    return 0;
}