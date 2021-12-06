#pragma once

#include "transport_catalogue.h"
#include "request_handler.h"
#include "json_reader.h"
#include "map_renderer.h"

#include <string>
#include <deque>
#include <iostream>
#include <fstream>
using namespace std::string_literals;

using namespace transport_system;
using namespace request_handler;
using namespace json;
using namespace map_renderer;
using namespace subjects;
using namespace subjects::obj;
using namespace subjects::info;

namespace Test
{
    // Функция для сравнения двух чисел double
    bool fequal(const double num1, const double num2)
    {
        return std::abs(num1 - num2) < 0.0001;
    }

    // Тест-функция для макроса ASSERT
    template <typename T>
    void AssertImpl(const T& t, const std::string& t_str, const std::string& file, const std::string& func,
        unsigned line, const std::string& hint) 
    {
        if (!t) {
            std::cerr << file << "(" << line << "): " << func << ": " << "ASSERT(" << t_str << ") failed.";
            if (!hint.empty()) {
                std::cerr << " Hint: " << hint;
            }
            std::cerr << std::endl;
            abort();
        }
    }
    #define ASSERT(expr) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, "")
    #define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __FILE__, __FUNCTION__, __LINE__, hint)

    // Тест-функция для макроса ASSERT_EQUAL
    template <typename T, typename U>
    void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
        const std::string& func, unsigned line, const std::string& hint) 
        {
        if (t != u) {
            std::cerr << std::boolalpha;
            std::cerr << file << "(" << line << "): " << func << ": ";
            std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
            std::cerr << t << " != " << u << ".";
            if (!hint.empty()) {
                std::cerr << " Hint: " << hint;
            }
            std::cerr << std::endl;
            abort();
        }
    }
    #define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")
    #define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

    // Функция для запуска теста через макрос RUN_TEST
    template <typename TestFunction>
    void RunTestImpl(TestFunction test_func, std::string test_function_string) {
        test_func();
        std::cout << test_function_string << " OK" << std::endl;
    }
    #define RUN_TEST(func)  RunTestImpl(func, #func);

    namespace Generator
    {
        namespace
        {
            const Stop* FindStopByName(const std::string& name, const std::deque<Stop>& stops)
            {
                for (const auto& el: stops) {
                    if (el.name == name) {
                        return &el;
                    }
                }
                return nullptr;
            }
        }

        std::deque<Stop> GenerateSomeStops()
        {
            std::deque<Stop> res;
            res.push_back({"Stop1"s, {43.587795, 39.716901}});  // Ривьерский мост
            res.push_back({"Stop2"s, {43.581969, 39.719848}});  // Морской вокзал
            res.push_back({"Stop3"s, {43.598701, 39.730623}});  // Электросети
            res.push_back({"Stop4"s, {43.585586, 39.733879}});  // Улица Докучаева
            res.push_back({"Stop5"s, {43.590317, 39.746833}});  // Улица Лизы Чайкиной
            return res;
        }
        std::deque<Bus> GenerateSomeBuses(std::deque<Stop>& stops)
        {
            std::deque<Bus> res;
            res.push_back({"14", {
                FindStopByName("Stop5"s, stops), 
                FindStopByName("Stop3"s, stops), 
                FindStopByName("Stop4"s, stops), 
                FindStopByName("Stop5"s, stops)
            }, true});
            res.push_back({"114", {
                FindStopByName("Stop2"s, stops), 
                FindStopByName("Stop1"s, stops)
            }, false});
            return res;
        }
    }

    namespace ModuleTests
    {
        void TestSVGPrintLines()
        {
            {
                TransportSystem ts;
                std::deque<Stop> stops = Generator::GenerateSomeStops();
                std::deque<Bus> buses = Generator::GenerateSomeBuses(stops);

                std::deque<const Stop*> pstops;
                for (const auto& el: stops) {
                    pstops.push_back(&el);
                    ts.AddStop(el);
                }
                
                std::deque<const Bus*> pbuses;
                for (const auto& el: buses) {
                    pbuses.push_back(&el);
                    ts.AddRoute(el);
                }

                std::deque<StopInfo> stops_info;
                for (const auto& el: pstops) {
                    stops_info.push_back(ts.GetStopInfoByStop(el));
                } 

                std::ofstream ofs("../test_output/TestSVGPrintLines.svg"s);
                map_renderer::PrintSVGMap(pbuses, pstops, ofs, stops_info);
            }
        }
    }

    void TestRendering()
    {
        using namespace ModuleTests;
        RUN_TEST(TestSVGPrintLines);
    }
}