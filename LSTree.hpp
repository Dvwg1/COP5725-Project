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


class ls_tree {
    public:
    //class for LSTree, collection of RTrees
    ls_tree(const string& dir); 

    map<string, b_plus_tree> levels;
    //vector<b_plus_tree> levels; 

    //used to get root and handler info for main
    int getRootPage()  { return root_page; }
    page_handler& getHandler()  { return handler; }

    b_plus_tree memoryTree;
    bool isMemoryTree = false; 

    void addTree(const string& baseDir);
    b_plus_tree& getTree(size_t index);
    size_t size() const; 
    //void addToTree(b_plus_tree& btree, int key, const Record& rec);
    void addToTree(const std::string& treeName, int key, const Record& rec); 

    void insertMemoryTree();
    


    private: 

    //b plus tree will have its own instance of handler
    page_handler handler;

    void saveRoot();

    //stores the root page id 
    int root_page;

    string baseDirectory; 

} ;

extern const string ROOT_META_FILE;