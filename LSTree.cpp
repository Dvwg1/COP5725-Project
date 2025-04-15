
#include "rtree.hpp"
#include "LSTree.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
//for default_random_engine
#include <random>
//for shuffle
#include <algorithm>
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

/*
//used to save the root page id for recals
void ls_tree::saveRoot() {

    //opens root.meta file for reading
    ofstream out(ROOT_META_FILE);

    //reads in the value
    out << root_page;
} */


//helper function to read last disk tree into memory
vector<Record> getRecords(b_plus_tree& tree) {
    vector<Record> result;
    char buffer[PAGE_SIZE];

    page_handler& handler = tree.getHandler();
    int current = tree.getRootPage();

    // Descend to first leaf
    while (true) {
        handler.readPage(current, buffer);

        int is_leaf;
        memcpy(&is_leaf, buffer, sizeof(int));
        if (is_leaf)
            break;

        internal_node* inode = reinterpret_cast<internal_node*>(buffer);
        current = inode->children[0];
    }

    // Traverse leaves
    while (current != INVALID_PAGE) {
        handler.readPage(current, buffer);
        leaf_node* leaf = reinterpret_cast<leaf_node*>(buffer);

        for (int i = 0; i < leaf->record_num; ++i) {
            result.push_back(leaf->records[i]);
        }

        current = leaf->next_leaf_page;
    }

    return result;
}





void ls_tree::insertMemoryTree(const string& dir) {
    //check if levels is empty before getting last tree
    if (!levels.empty()) {
        b_plus_tree& lastTree = levels.rbegin()->second; 
        vector<Record> records = getRecords(lastTree); 
        //memoryTree = b_plus_tree("inMemoryTree"); 
        isMemoryTree = true;
        cout << "checking memoryTree " << endl;

        for (size_t i = 0; i < records.size(); i++)
        {
            /* code */
            memoryTree.insert(records[i].hilbert, records[i]);
        }
        

        //now remove from disk
        auto lastDiskTree = levels.rbegin();
        string lastDiskTreeName = lastDiskTree->first;


        fs::remove_all(lastDiskTreeName);

        //remove in levels
        levels.erase(lastDiskTreeName); 
        
    }
}


vector<Record> ls_tree::querying(int low, int high, long unsigned int k) { //int k

    vector<Record> results;
    //
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    if (isMemoryTree) {
        cout << "inMemorytree" << endl;
        cout << memoryTree.getRootPage() << " yikes" << endl; 
        //if(low < maxMin.back().max_hilbert || high > maxMin.back().min_hilbert)
        if (low <= maxMin.back().max_hilbert && high >= maxMin.back().min_hilbert)
        {
            //query the memory tree first
            cout << "in if statement" << endl; 
            vector<Record> memoryResults = memoryTree.rangeQueryR(low, high);
            cout << "ending rangeQuery and onto shuffle" << endl; 
            //vector<Record> rs = memoryTree.rangeQuery(32876, 100000);
            //vector<Record> rs = memoryTree.rangeQuery(0, 100000);
            
            shuffle (memoryResults.begin(), memoryResults.end(), default_random_engine(seed));
            //std::shuffle(memResults.begin(), memResults.end(), std::mt19937{std::random_device{}()});
            cout << "size of memoryTree: " << memoryResults.size() << endl; 

            //check if memoryTree works
            cout << "Results from inMemoryTree:\n";
            for (size_t i = 0; i < memoryResults.size(); i++)
            {
                cout << "ID: " << memoryResults[i].id
                << ", Lat: " << memoryResults[i].lat
                << ", Lon: " << memoryResults[i].lon
                << ", Time: " << memoryResults[i].timestamp
                << ", Hilbert: " << memoryResults[i].hilbert << endl; 

                results.push_back(memoryResults[i]); 
                if(results.size() == k) {
                    //up to k in results
                    return results; 
                }
                
            }
        }
    }

    //now check disk based trees
    //i is for going from smallest tree to biggest tree (reverse)
    cout << "done checking MemTree and going to disk trees" << endl;
    size_t i = maxMin.size() - 1; 
    //looping through a map
    //https://stackoverflow.com/questions/26281979/how-do-you-loop-through-a-stdmap
    for (auto it = levels.rbegin(); it!=levels.rend(); it++, --i) {

        max_min_hilbert& treeRange = maxMin[i];
        if(low > treeRange.max_hilbert || high < treeRange.min_hilbert) { continue;}
    
        //if (it->first == memoryTree)
        vector<Record> diskResults = it->second.rangeQueryR(low, high);
        std::shuffle(diskResults.begin(), diskResults.end(), default_random_engine(seed));
        for (size_t i = 0; i < diskResults.size(); i++)
        {
            /*cout << " disk ID: " << diskResults[i].id
            << ", Lat: " << diskResults[i].lat
            << ", Lon: " << diskResults[i].lon
            << ", Time: " << diskResults[i].timestamp
            << ", Hilbert: " << diskResults[i].hilbert << endl; */

            results.push_back(diskResults[i]); 
            if(results.size() == k) {
                //up to k in results
                return results; 
            }
            
        }
    
    }

    return results; 
}


void ls_tree::insertMoreRecords(const Record& rec) {
   //used to insert records after tree is built
   cout << "size of levels: " << levels.size() << endl ;
   //return; 

   //insert 5000 records from beginning of file
   int hilbert = rec.hilbert;
   int directoryCounter = 0; 
   string LSTreeDir = "ls_tree_pages/btree";
   string strDCounter = to_string(directoryCounter);
   string directroy = LSTreeDir + strDCounter;

   addToTree(directroy, hilbert, rec);

   directoryCounter++; 

   for (size_t i = 1; i < levels.size(); i++)
   {
    int flip = rand() % 2; 
    if (flip == 1){
        cout << "coin flip at: " << i << " is a: " << flip << endl; 
        break;
        
    }
    else { 
        //directoryCounter++;
        strDCounter = to_string(directoryCounter);
        //directroy = LSTreeDir + strDCounter;
        directroy = LSTreeDir + to_string(i);
        addToTree(directroy, hilbert, rec);
    }

   }
   

}


void ls_tree::removeHilbert(const Record& rec) {
    //to remove from ls-tree based on hilbert value
    /*for (size_t i = 0; i < levels.size(); i++) {
        levels
    }*/

    for (auto it = levels.begin(); it!=levels.end(); it++) {
        it->second.remove(rec.hilbert); 
    }

}