//LSTree.hpp


#include "rtree.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>



using namespace std; 

struct max_min_hilbert {
    long int min_hilbert;
    long int max_hilbert; 
} ; 


class ls_tree {
    public:
    //class for LSTree, collection of RTrees
    ls_tree(const string& dir); 

    map<string, b_plus_tree> levels;
    vector<max_min_hilbert> maxMin; 

    //used to get root and handler info for main
    int getRootPage()  { return root_page; }
    page_handler& getHandler()  { return handler; }

    b_plus_tree memoryTree;
    bool isMemoryTree = false; 

    b_plus_tree& getTree(size_t index);
    size_t size() const; 
    //void addToTree(b_plus_tree& btree, int key, const Record& rec);
    void addToTree(const string& treeName, int key, const Record& rec); 

    void insertMemoryTree(const string& dir);

    vector<Record> querying(int low, int high, long unsigned int k); //int k 

    void insertMoreRecords(const Record& rec); 

    void removeHilbert(const Record& rec);

    vector<Record> getRecords(b_plus_tree& tree); 
    


    private: 

    //b plus tree will have its own instance of handler
    page_handler handler;

    void saveRoot();

    //stores the root page id 
    int root_page;

    string baseDirectory; 

} ;

extern const string ROOT_META_FILE;