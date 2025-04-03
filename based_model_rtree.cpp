//--- Hilbert R-Tree ---

/*
This is used to construct a base model R-tree using the h_rtree header files.
*/

//h_rtree header files
#include "rtree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>

using namespace std;

int main() {

    static_assert(sizeof(Record) <= PAGE_SIZE, "Record too large!");
    std::cout << "Size of Record: " << sizeof(Record) << " bytes" << std::endl;

    std::string inputFile;
    std::cout << "Enter CSV file path: ";
    std::getline(std::cin, inputFile);

    std::ifstream file(inputFile);
    if (!file.is_open()) {
        std::cerr << "Failed to open file." << std::endl;
        return 1;
    }

    b_plus_tree tree("tree_pages");

    std::string line;
    std::getline(file, line); // Skip header

        while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string idStr, latStr, lonStr, tsStr, hStr;

        if (std::getline(ss, idStr, ',') &&
            std::getline(ss, latStr, ',') &&
            std::getline(ss, lonStr, ',') &&
            std::getline(ss, tsStr, ',') &&
            std::getline(ss, hStr, ',')) {

            try {
                Record r;
                strncpy(r.id, idStr.c_str(), sizeof(r.id));
                r.id[sizeof(r.id) - 1] = '\0';

                r.lon = std::stof(latStr);
                r.lat = std::stof(lonStr);
                strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                r.hilbert = std::stoi(hStr);

                tree.insert(r.hilbert, r);
            } 
            
            //error catching for debugging
            catch (const std::exception& e) {
                std::cerr << "Invalid line (skipping): " << line << "\\n";
                std::cerr << "  Error: " << e.what() << "\\n";
            }
        } 
    }


    file.close();
    std::cout << "Tree built from CSV and stored on disk.\n";

    //tree.exportToDot("tree.dot");
    //std::cout << "DOT file generated: tree.dot\n";

    return 0;
}
