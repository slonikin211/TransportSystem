#include "json_reader.h"
#include "map_renderer.h"
#include "svg.h"
#include "request_handler.h"

#include <iostream>

using namespace std::literals;

int main() 
{
	renderer::MapRenderer mr;
	transport::TransportCatalogue db;

	request_handler::RequestHandler rh(db, mr);
	json_reader::JsonReader js_reader(rh);

	js_reader.Start(std::cin, std::cout);
}