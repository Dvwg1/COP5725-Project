//--- RS-Tree ---

/*
This is used to construct an RS-tree using the h_rtree header files.
Used in some experiments as is
*/

//h_rtree header files
#include "RStree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>

using namespace std;


int main() {

    string inputFile;
    cout << "Enter CSV file path: ";
    getline(cin, inputFile);

    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        return 1;
    }

    //starts the timer, at this point the file should have been found
	auto start = chrono::high_resolution_clock::now();

    //initialize memory-based RS-tree
    b_plus_tree tree;

    string line;
    getline(file, line);

    while (getline(file, line)) {
        stringstream ss(line);
        string idStr, latStr, lonStr, tsStr, hStr;

        if (getline(ss, idStr, ',') &&
            getline(ss, latStr, ',') &&
            getline(ss, lonStr, ',') &&
            getline(ss, tsStr, ',') &&
            getline(ss, hStr, ',')) {

            try {
                Record r;
                strncpy(r.id, idStr.c_str(), sizeof(r.id));
                r.id[sizeof(r.id) - 1] = '\0';

                r.lon = stof(latStr);
                r.lat = stof(lonStr);
                strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                r.hilbert = stoi(hStr);

                tree.insert(r.hilbert, r);
            } 
            
            //error catching for debugging
            catch (const exception& e) {
                cerr << "Invalid line (skipping): " << line << "\\n";
                cerr << "  Error: " << e.what() << "\\n";
            }
        } 
    }


    file.close();
    //ends timer after sorted data set is complete, calculates elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_time = end - start;
    cout << "RS-Tree built from CSV and stored on disk.\n";
	cout << "Total sorting time elapsed: " << total_time.count() << " seconds" << endl;

    tree.printTree();

    //official goobab function tests
    /*
    tree.remove(1364);

    vector<Record> results = tree.rangeQuery(1300, 6000);


    for (const Record& r : results) {

        cout << "  [" << r.hilbert << "] "
        << r.id << " | "
        << "(" << r.lon << ", " << r.lat << ") | "
        << r.timestamp << "\n";

    }
    */

    return 0;
}
