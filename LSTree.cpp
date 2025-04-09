
#include "rtree.hpp"
#include "LSTree.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <filesystem>



using namespace std; 

namespace fs = filesystem;

//root.meta is used for page directory organization
const string LS_ROOT_META_FILE = "ls_tree_pages/root.meta";


//constructor
ls_tree::ls_tree(const string& dir) : memoryTree("inMemoryTree"), handler(dir) {

    fs::create_directory(dir);
}




/*void ls_tree::addToTree(b_plus_tree& btree, int key, const Record& rec) {
    //if btree exists, then insert key and rec
    //else create btree and insert it
    
}*/

void ls_tree::addToTree(const string& treeName, int key, const Record& rec) {
    // Check if the tree exists
    // if (levels.find(treeName) == levels.end()) {
    //     // Create new tree if it doesn't exist
    //     b_plus_tree btree(treeName); 
    //     btree.insert(key,rec);
    //     levels.insert({treeName, btree}); 
    //     //levels[treeName] = btree;  // Assuming this constructor sets up the tree
    // } else {
    //     //cout << "this is wrong" << endl; 
    //     levels[treeName].insert(key, rec);
        
    // }
    // Insert into the existing tree
    //levels[treeName].insert(key, rec);

    auto it = levels.find(treeName);
    if (it == levels.end()) {
        b_plus_tree btree(treeName);
        btree.insert(key, rec);
        levels.insert({treeName, btree});
    } else {
        it->second.insert(key, rec);
}

}



void ls_tree::addTree(const string& treeName) {
    string treePath = baseDirectory + "/" + treeName;
    if (!fs::exists(treePath)) {
        fs::create_directory(treePath);
    }

    // Pass the treePath to RTree constructor if needed
    b_plus_tree newTree(treePath); 
    //levels.push_back(newTree);
}

/*
//used to save the root page id for recals
void ls_tree::saveRoot() {

    //opens root.meta file for reading
    ofstream out(ROOT_META_FILE);

    //reads in the value
    out << root_page;
} */

void ls_tree::insertMemoryTree() {
    //check if levels is empty before getting last tree
    if (!levels.empty()) {
        memoryTree = levels.rbegin()->second; 
        isMemoryTree = true;
        cout << "checking memoryTree : " << endl;

        //cout << "memoryTree root page: " << memoryTree.getRootPage() << endl; 
        
        vector<Record> rs = memoryTree.rangeQuery(32876, 100000);

        cout << "Results from inMemoryTree:\n";
        for (const Record& rec : rs) {
            cout << "ID: " << rec.id
                    << ", Lat: " << rec.lat
                    << ", Lon: " << rec.lon
                    << ", Time: " << rec.timestamp
                    << ", Hilbert: " << rec.hilbert
                    << "\n";
        }



        
        // cout << leaf->records[i].id << endl;
        // cout << leaf->records[i].lat << endl;
        // cout << leaf->records[i].lon << endl;
        // cout << leaf->records[i].timestamp << endl; 
        
    }
}

