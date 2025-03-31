#ifndef HILBERT
#define HILBERT

// --Hilbert Value calculation code--
/*
References: I. Kamel and C. Faloutsos. Hilbert R-tree: An Improved R-tree using Fractals. In VLDB, 1994
            Yaltirakli, Gokberk. Hilbert Curve. gkbrk.com, 2025. https://www.gkbrk.com/wiki/hilbert-curve/

Usage: To be used for the R-tree and for csv record sorting
for calculation, run the max/min function to obtain the needed max/min values latitude and longitudes. Then,
to get the hilbert value for a record, use the coords_to_hilbert_value() functions, with longitude, latitude, 
max longitude, max latitude, and p as values. p is recommended to be 8

Explanation: to calculate hilbert values, coordinates must be normalized into a 2D grid as described in the paper,
of which the size is determined by a given power p. The normalized coordinates are then converted to a hilbert 
value, using c++ code adapted from Yaltirakli's work. Essentially, the code will recurisvely traverse the given
coordinate's quadrant and subquadarnarts, rotating when needed, until the final correct value is found. 
min and max longitudes and latitudes are needed to determine the size of the normalized 2D grid.
*/

//used for input and data manipulation
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

//dyanmic data structures and data types used
#include <vector>
#include <queue>
#include <cstdint>
#include <string>

//needed for calculations
#include <limits>
#include <algorithm>
#include <cmath>
#include <utility>

//global variables
extern double lat_min;
extern double lat_max;
extern double lon_min;
extern double lon_max;

//used to obtain the max and min longitude and latitiude from a specified csv
void max_and_min_finder(const std::string& filename) {

	//attempts to open specified value - gives warning if can't
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "ERROR: .csv file not found/can't be opened! " << filename << '\n';
        return;
    }

    //stores line from csv
    std::string line;

    //used to mark first read instance
    bool first = true;

    //used to skip the header line
    std::getline(file, line);

    //iterate through the file line by line
    while (std::getline(file, line)) {
        std::stringstream ss(line);

      	//variables to store latitude and longitude. id is stored as it is skipped
        std::string lat_str, lon_str, id_str;

        //reading in the values line by line
        if (!std::getline(ss, id_str, ',')) continue;
        if (!std::getline(ss, lat_str, ',')) continue;
        if (!std::getline(ss, lon_str, ',')) continue;

        //convert string lats and lons to string
        double lat = std::stod(lat_str);
        double lon = std::stod(lon_str);

        //set the first instances as the max
        if (first) {
            lat_min = lat_max = lat;
            lon_min = lon_max = lon;
            first = false;
        } 

        //after the first instance, check each line to see if a new record
        //if lowering/higher. By the end, only the min and max longitudes
        //and latitudes should remain
        else {
            if (lat < lat_min) lat_min = lat;
            if (lat > lat_max) lat_max = lat;
            if (lon < lon_min) lon_min = lon;
            if (lon > lon_max) lon_max = lon;
        }
    }

    file.close();
}

//normalizes latitude and longitidue coordinate into a 2D grid (0 to 2^p - 1)
//p can be changed to any value, though Kamel & Faloutsos seem to recommend 8 
std::pair<int, int> normalize_coords(double latitude, double longitude,double lat_min, double lat_max,
                                     double lon_min, double lon_max, int p) {

	//calculates 2^p and sets it equal to n
    int n = 1 << p;  

    //normalization calculation, using the min and max values to ensure all possible
    //longitudes and latitudes are included within the grid
    double lat_norm = (latitude - lat_min) / (lat_max - lat_min);
    int y = static_cast<int>(lat_norm * (n - 1));

    double lon_norm = (longitude - lon_min) / (lon_max - lon_min);
    int x = static_cast<int>(lon_norm * (n - 1));


    return {x, y};
}

//returns hilbert value from given points
//adapted from Yaltirakli's Python code
int xy2d(int n, int x, int y) {

    int d = 0;
    for (int s = n / 2; s > 0; s /= 2) {
        int rx = (x & s) > 0;
        int ry = (y & s) > 0;
        d += s * s * ((3 * rx) ^ ry);

        //rotation functionality
        if (ry == 0) {
            if (rx == 1) {
                x = n - 1 - x;
                y = n - 1 - y;
            }
            std::swap(x, y);
        }
    }
    return d;
}

//normalizes the given coords and then uses the xy2d function to produce the hilbert value
int coords_to_hilbert_value(double latitude, double longitude, double lat_min, double lat_max,
                                double lon_min, double lon_max, int p) {

    auto [x, y] = normalize_coords(latitude, longitude, lat_min, lat_max, lon_min, lon_max, p);
    int n = 1 << p;
    return xy2d(n, x, y);
}


#endif