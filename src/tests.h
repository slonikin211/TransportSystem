#pragma once

#include <string>
#include <cassert>

#include "input_reader.h"
#include "stat_reader.h"

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

            Bus bus1 = InitBusFromQuery(ts, to_parse1);
            Bus bus2 = InitBusFromQuery(ts, to_parse2); 

            assert(bus1.name == "256"s);
            assert(bus2.name == "750"s);        
        }
    }

    // Test main queries to TransportSysten after init
    void TestStatReader()
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;
        // Test1 Check correct getting bus info
        {
            TransportSystem ts;
            std::string to_parse1 = "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"s;
            std::string to_parse2 = "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"s;
            std::string to_parse3 = "Bus 777: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya"s;
            std::string to_parse4 = "Bus 751: Tolstopaltsevo - Marushkino - Rasskazovka - Tolstopaltsevo"s;

            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Zapadnoye: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryusinka: 56.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Universam: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Tovarnaya: 56.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Biryulyovo Passazhirskaya: 55.611087, 37.208290"s));

            ts.AddStop(GetInitStopFromQuery("Stop Tolstopaltsevo: 56.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Marushkino: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop Rasskazovka: 56.611087, 37.208290"s));


            ts.AddRoute(InitBusFromQuery(ts, to_parse1));
            ts.AddRoute(InitBusFromQuery(ts, to_parse2));
            ts.AddRoute(InitBusFromQuery(ts, to_parse3));
            ts.AddRoute(InitBusFromQuery(ts, to_parse4));

            BusInfo info1 = GetBusInfo(ts, "256"sv);
            assert(info1.amount_of_stops == 6u); 
            assert(info1.amount_of_unique_stops == 5u);           
            
            BusInfo info2 = GetBusInfo(ts, "750"sv);
            assert(info2.amount_of_stops == 5u); 
            assert(info2.amount_of_unique_stops == 3u);        
            
            BusInfo info3= GetBusInfo(ts, "777"sv);
            assert(info3.amount_of_stops == 6u); 
            assert(info3.amount_of_unique_stops == 5u);   
            
            BusInfo info4 = GetBusInfo(ts, "751"sv);
            assert(info4.amount_of_stops == 7u); 
            assert(info4.amount_of_unique_stops == 4u);  
        }

        // Test2 Check Input and Output from Example
        {
            TransportSystem system;

            // Init
            std::deque<std::string> inputInitDB = {
                "Stop Tolstopaltsevo: 55.611087, 37.208290"s,
                "Stop Marushkino: 55.595884, 37.209755"s,
                "Bus 256: Biryulyovo Zapadnoye > Biryusinka > Universam > Biryulyovo Tovarnaya > Biryulyovo Passazhirskaya > Biryulyovo Zapadnoye"s,
                "Bus 750: Tolstopaltsevo - Marushkino - Rasskazovka"s,
                "Stop Rasskazovka: 55.632761, 37.333324"s,
                "Stop Biryulyovo Zapadnoye: 55.574371, 37.651700"s,
                "Stop Biryusinka: 55.581065, 37.648390"s,
                "Stop Universam: 55.587655, 37.645687"s,
                "Stop Biryulyovo Tovarnaya: 55.592028, 37.653656"s,
                "Stop Biryulyovo Passazhirskaya: 55.580999, 37.659164"s
            };
            ProcessInitQueries(system, inputInitDB);

            // Queries
            std::deque<std::string> inputQueryDB = {
                "Bus 256",
                "Bus 750",
                "Bus 751"
            };
            auto res = ProcessDBQueries(system, inputQueryDB);
            assert(res == "Bus 256: 6 stops on route, 5 unique stops, 4371.02 route length\nBus 750: 5 stops on route, 3 unique stops, 20939.5 route length\nBus 751: not found\n");
        }
    }

    void TestCriticalInput()
    {
        using namespace std::string_literals;
        using namespace std::string_view_literals;

        // Test1. Stop formats
        {
            TransportSystem ts;

            ts.AddStop(GetInitStopFromQuery("Stop X: 55.611087, 37.208290"s));
            assert(ts.FindStopByName("X"s)->name == "X"s);
            assert(dequal(ts.FindStopByName("X"s)->coordinates.lat, 55.611087));
            assert(dequal(ts.FindStopByName("X"s)->coordinates.lng, 37.208290));
            
            ts.AddStop(GetInitStopFromQuery("Stop  Y : 55.611087, 37.208290"s));
            assert(ts.FindStopByName("Y"s)->name == "Y"s);
            assert(dequal(ts.FindStopByName("Y"s)->coordinates.lat, 55.611087));
            assert(dequal(ts.FindStopByName("Y"s)->coordinates.lng, 37.208290));

            ts.AddStop(GetInitStopFromQuery("Stop A B: 55.611087, 37.208290"s));
            assert(ts.FindStopByName("A B"s)->name == "A B"s);
            assert(dequal(ts.FindStopByName("A B"s)->coordinates.lat, 55.611087));
            assert(dequal(ts.FindStopByName("A B"s)->coordinates.lng, 37.208290));

            ts.AddStop(GetInitStopFromQuery("Stop   A 123 B  :   55.611087,   37.208290   "s));
            assert(ts.FindStopByName("A 123 B"s)->name == "A 123 B"s);
            assert(dequal(ts.FindStopByName("A 123 B"s)->coordinates.lat, 55.611087));
            assert(dequal(ts.FindStopByName("A 123 B"s)->coordinates.lng, 37.208290));

            ts.AddStop(GetInitStopFromQuery("Stop F:55.611087,37.208290"s));
            assert(ts.FindStopByName("F"s)->name == "F"s);
            assert(dequal(ts.FindStopByName("F"s)->coordinates.lat, 55.611087));
            assert(dequal(ts.FindStopByName("F"s)->coordinates.lng, 37.208290));
        }

        // Test2 Routes formats
        {
            TransportSystem ts;
            ts.AddStop(GetInitStopFromQuery("Stop A1: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop A2: 55.595884, 37.209755"s));
            ts.AddStop(GetInitStopFromQuery("Stop A3: 55.632761, 37.333324"s));
            ts.AddStop(GetInitStopFromQuery("Stop A4: 55.574371, 37.651700"s));
            ts.AddStop(GetInitStopFromQuery("Stop A5: 55.581065, 37.648390"s));
            ts.AddStop(GetInitStopFromQuery("Stop AA6 AA6  AA6 AA6: 55.581065, 37.648390"s));

            {
                ts.AddRoute(InitBusFromQuery(ts, "Bus X: A1 > A2"s));
                assert(ts.FindRouteByBusName("X"s)->name == "X"s);    
                assert(ts.FindRouteByBusName("X"s)->cyclic_route == true);   
            }
            {
                ts.AddRoute(InitBusFromQuery(ts, "Bus A B : A1 > A2"s));
                assert(ts.FindRouteByBusName("A B"s)->name == "A B"s);    
                assert(ts.FindRouteByBusName("A B"s)->cyclic_route == true);   
            }
            {
                ts.AddRoute(InitBusFromQuery(ts, "Bus YYY : A1 > A2 > AA6 AA6  AA6 AA6"s));
                assert(ts.FindRouteByBusName("YYY"s)->name == "YYY"s);    
                assert(ts.FindRouteByBusName("YYY"s)->cyclic_route == true);   
            }
        }

        // Test3 THE MOST CRITICAL (FROM GENERATED TESTS)
        {
            TransportSystem ts;
            ts.AddStop(GetInitStopFromQuery("Stop A1: 55.611087, 37.208290"s));
            ts.AddStop(GetInitStopFromQuery("Stop A2: 55.595884, 37.209755"s));
            ts.AddStop(GetInitStopFromQuery("Stop A3: 55.632761, 37.333324"s));
            ts.AddStop(GetInitStopFromQuery("Stop A4: 55.574371, 37.651700"s));
            ts.AddStop(GetInitStopFromQuery("Stop A5: 55.581065, 37.648390"s));
            ts.AddStop(GetInitStopFromQuery("Stop AA6 AA6  AA6 AA6: 55.581065, 37.648390"s));
            {
                ts.AddRoute(InitBusFromQuery(ts, "Bus X: A1 > A2 > A3 > A4 > A3 > A5"s));
                assert(ts.FindRouteByBusName("X"s)->name == "X"s);    
                assert(ts.FindRouteByBusName("X"s)->cyclic_route == true);

                auto info = GetBusInfo(ts, "X");
                assert(info.amount_of_stops == 7u);
                assert(info.amount_of_unique_stops == 4u);   
            }
        }
    }

    // TEST MODULE ENDS HERE

    void TestTransportSystem()
    {
        // Tests
        RUN_TEST(TestInputReader);
        RUN_TEST(TestStatReader);
        RUN_TEST(TestCriticalInput);
    }
} // namespace Test