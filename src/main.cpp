#include "json_reader.h"
#include "map_renderer.h"
#include "svg.h"
#include "request_handler.h"
#include <iostream>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        return 1;
    }

	renderer::MapRenderer mr;
	transport::TransportCatalogue db;
	request_handler::RequestHandler rh(db, mr);
	json_reader::JsonReader js_reader(rh);

    const std::string_view mode(argv[1]);
    // const std::string_view mode = "process_requests"sv;
    if (mode == "make_base"sv) {
        js_reader.MakeBase(std::cin);
    } else if (mode == "process_requests"sv) {
        js_reader.ProcessRequests(std::cin, std::cout);
    } else {
        PrintUsage();
        return 1;
    }
}