//--- RS-Tree ---

/*
This is used to construct an RS-tree using the h_rtree header files.
Used in some experiments as is
*/

// header files
#include "RStree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <cstdlib>  

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
    b_plus_tree tree("RStree_pages");

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

                r.lon = stof(lonStr);
                r.lat = stof(latStr);
                strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                r.hilbert = stoi(hStr);

                tree.insert(r.hilbert, r, true);
            } 
            
            //error catching for debugging
            catch (const exception& e) {
                cerr << "Invalid line (skipping): " << line << "\\n";
                cerr << "  Error: " << e.what() << "\\n";
            }
        } 
    }


    file.close();

    //do sampling on tree before it can be said to have finished construction
    tree.buildAllSamples();

    //ends timer after sorted data set is complete, calculates elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_time = end - start;
    cout << "RS-Tree built from CSV and stored on disk.\n";
	cout << "Total sorting time elapsed: " << total_time.count() << " seconds" << endl;




    //can used printree and goobab testing to show during demo how it works
    tree.printTree();
    cout << "before removals" << endl;

    //official goobab function tests
    
    tree.remove(5544);
    tree.remove(5544);
    tree.remove(5546);
    tree.remove(5717);
    tree.remove(9851);
    tree.remove(9859);
    tree.remove(9859);

    cout << "after removals" << endl;
    tree.printTree();

/*
    auto results= tree.rangeQuery(1300, 6000);

    for (const Record& r : results) {

        cout << "  [" << r.hilbert << "] "
        << r.id << " | "
        << "(" << r.lon << ", " << r.lat << ") | "
        << r.timestamp << "\n";

    }
    */

    /*
    Record test;
    strcpy(test.id, "test_id_0001");
    test.lon = 99.9999f;
    test.lat = 99.9999f;
    strcpy(test.timestamp, "2099-12-31 23:59:59");
    test.hilbert = 999999;  // very distinct

    tree.insert(test.hilbert, test, false);

    tree.printTree();
    */



    //cleans up disk directory
    int status = system("rm -rf RStree_pages");
    if (status == 0)
        cout << "removed RStree_pages" << endl; 
    else
        cerr << "error in cleanup" << endl;
    

    return 0;
}
