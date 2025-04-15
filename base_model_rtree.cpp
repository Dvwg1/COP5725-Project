//--- Hilbert R-Tree ---

/*
This is used to construct a base model R-tree using the h_rtree header files.
Used in some experiments as is
*/

//h_rtree header files
#include "rtree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>

using namespace std;

//debugging function
//used to confirm that the children are actually child page IDs
void print_children( b_plus_tree& tree) {

    //creates a queue to hold nodes, adds the root page for searching
    queue<int> node_queue;
    node_queue.push(tree.getRootPage());

    //buffer initialize
    char buffer[PAGE_SIZE];

    //loops until the entire queue has been iterated
    while (!node_queue.empty()) {

        //set page id to the first node, and remove
        int page_id = node_queue.front();
        node_queue.pop();

        //use the id to read its value
        tree.getHandler().readPage(page_id, buffer);

        //get the leaf/internal status
        int is_leaf;
        memcpy(&is_leaf, buffer, sizeof(int));

        //ends loop if not leaf
        if (is_leaf) 
            continue;

        //brings out the node, and prints its keys and children
        internal_node* node = reinterpret_cast<internal_node*>(buffer);
        cout << "Internal Node Page ID: " << page_id << endl;

        cout << "  Keys: ";
        for (int i = 0; i < node->numKeys; ++i)
            cout << node->keys[i] << " ";
            
        cout << "\n  Children: ";
        for (int i = 0; i <= node->numKeys; ++i)
            cout << node->children[i] << " ";
        cout << "\n\n";

        for (int i = 0; i <= node->numKeys; ++i)
            node_queue.push(node->children[i]);
    }
}
//end of debugging function


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

    b_plus_tree tree("tree_pages");

    string line;
    getline(file, line); // Skip header

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
    cout << "Tree built from CSV and stored on disk.\n";
	cout << "Total sorting time elapsed: " << total_time.count() << " seconds" << endl;

    //children debugging function call
    //print_children(tree);

    tree.removeR(1364);


    tree.exportToDot("tree.dot");
    cout << "DOT file generated: tree.dot\n";


    return 0;
}
