#pragma once

#include <string>
#include <cassert>

#include "input_reader.h"

namespace Test
{
    // Function for comparing doubles
    bool dequal(const double num1, const double num2)
    {
        return std::abs(num1 - num2) < 0.0001;
    }

    template <typename TestFunction>
    void RunTestImpl(TestFunction test_func, std::string test_function_string) 
    {
            test_func();
            std::cout << test_function_string << ": " << "OK" << std::endl;
    }
    #define RUN_TEST(func)  RunTestImpl(func, #func);


    // TEST MODULE STARTS HERE

    // Test init of TransportSystem
    void TestInputReader()
    {
        using namespace std::string_literals;
        // Test1 correct parse standart string STOP
        {
            std::string to_parse = "Stop Tolstopaltsevo: 55.611087, 37.208290"s;
            Stop stop = GetInitStopFromQuery(to_parse);

            assert(stop.name == "Tolstopaltsevo"s);
            assert(dequal(55.611087, stop.coordinates.lat));
            assert(dequal(37.208290, stop.coordinates.lng));
        }

        // Test2 find Stop
        {
            TransportSystem ts;
            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Zapadnoye: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryusinka: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Universam: 55.611087, 37.208290"s));

            const Stop* stop1 = ts.FindStopByName("Biryulyovo Zapadnoye"s);
            assert(stop1->name == "Biryulyovo Zapadnoye"s);
            assert(dequal(stop1->coordinates.lat, 55.611087));
            assert(dequal(stop1->coordinates.lng, 37.208290));

            const Stop* stop2 = ts.FindStopByName("Biryusinka"s);
            assert(stop2->name == "Biryusinka"s);
            assert(dequal(stop2->coordinates.lat, 55.611087));
            assert(dequal(stop2->coordinates.lng, 37.208290));

            const Stop* stop3 = ts.FindStopByName("Universam"s);
            assert(stop3->name == "Universam"s);
            assert(dequal(stop3->coordinates.lat, 55.611087));
            assert(dequal(stop3->coordinates.lng, 37.208290));

            const Stop* stop4 = ts.FindStopByName("No stop here"s);
            assert(stop4 == nullptr);
        }

        // Test3 correct parse standart string BUS
        {
            TransportSystem ts;
            std::string to_parse1 = "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"s;
            std::string to_parse2 = "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"s;

            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Zapadnoye: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryusinka: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Universam: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Tovarnaya: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Passazhirskaya : 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Tolstopaltsevo: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Marushkino: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Rasskazovka: 55.611087, 37.208290"s));

            Bus bus1 = GetInitBusFromQuery(ts, to_parse1);
            Bus bus2 = GetInitBusFromQuery(ts, to_parse2); 

            assert(bus1.name == "256"s);
            assert(bus2.name == "750"s);        
        }
    }

    // TEST MODULE ENDS HERE

    void TestTransportSystem()
    {
        // Tests
        RUN_TEST(TestInputReader);
    }
} // namespace Test
