#include "geo.h"
#include <math.h>

const inline int EarthRadius = 6371000;

inline bool dequal(const double num1, const double num2)
{
    return std::fabs(num1 - num2) < 0.0001;
}

double subjects::geo::ComputeDistance(Coordinates from, Coordinates to) 
{
    using namespace std;
    if (dequal(from.lat, to.lat) && dequal(from.lng, to.lng))
    {
        return 0.0;
    }
    static const double dr = 3.1415926535 / 180.;
    return acos(
        sin(from.lat * dr) * 
        sin(to.lat * dr) + 
        cos(from.lat * dr) * 
        cos(to.lat * dr) * 
        cos(fabs(from.lng - to.lng) * dr)
    ) * EarthRadius;
}
