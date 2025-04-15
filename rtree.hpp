// --- Hilbert R-Tree ---

/*
References:
https://www.geeksforgeeks.org/cpp-program-to-implement-b-plus-tree/

https://github.com/andylamp/BPlusTree
https://github.com/myui/btree4j

The first link was used for the architecture needed to construct a B+ tree, as all the 
R-tree is is a Hilbert Value sorted B+ Tree. We of course adapated and modified the tree into the Hilbert R-Tree.
We first implemented this as an internal tree.

The last two links are used as inspiration for the disk based implementation of the B+ tree.

--- Hilbert R-Tree function and struct declarations ---

*/


//a check to make sure that this header file is only included once
#pragma once

//needed for data manipulation
#include <vector>
#include <string>

using namespace std;

//Note: pages represent a node

//defines the Page size to be 8000 bytes, as Wang et al. have their page sizes set to 8 KB
constexpr size_t PAGE_SIZE = 8000;

//Arbritary amount of records to be included in a node, set to 100 to ensure no overflow
constexpr int MAX_LEAF_RECORDS = 100;

//max fanout of 16, so there is to be 0-15 keys per node
constexpr int MAX_INTERNAL_KEYS = 15;

//a -1 page will be used as an invalid check
constexpr int INVALID_PAGE = -1;

//defines external variable name for needed root file 
extern const string ROOT_META_FILE;

//used for packing alignment - memory issues without
#pragma pack(push, 1)

//structure used to store records from sorted csv, including hilbert
struct Record {

    //12 bytes, or 24 characters plus 1 for \n
    //easier to store as a char
    char id[25];

    float lon;
    float lat;

    //due to some memory spacing error, we determined 29 is a good size
    char timestamp[29];

    int hilbert;
};

//used in conjunction with push
#pragma pack(pop)


//dynamic record size assignment used in testing, we kept anyways
constexpr size_t RECORD_SIZE = sizeof(Record);  

//leaf node, stores records
struct leaf_node {

    //bool: 1 for leaf
    int is_leaf = 1;

    int record_num = 0;
    int next_leaf_page = INVALID_PAGE;

    //max amount of records per leaf node
    Record records[MAX_LEAF_RECORDS];
};

//error checking 
static_assert(sizeof(leaf_node) <= PAGE_SIZE, "leaf_node exceeds page size");

//internal node, stores key (used as MBB) and children nodes
struct internal_node {

    //bool: 0 for internal
    int is_leaf = 0;
    int numKeys = 0;
  
    //children and keys
    int keys[MAX_INTERNAL_KEYS];
    int children[MAX_INTERNAL_KEYS + 1];
};

//error checking
static_assert(sizeof(internal_node) <= PAGE_SIZE, "internal_node exceeds page size");

//class used to handle pages for inserts, writes, reads. base of I/O functionality
class page_handler {

public:

    //constructor
    page_handler(const string& dir);

    //gets the next page
    int pageIncrementer();
    
    //disk read/write operations
    void writePage(int pageID, const void* data, size_t dataSize);
    void readPage(int pageID, void* buffer);

    //gets the page path for further operations
    string getPagePath(int pageID);

private:
    //variables
    string directory;
    int next_page_id;

    void loadNextPageID();
};

//our B+ tree class, containing its needed functions to be created and operated upon
class b_plus_tree {
public:

    //constructor
    b_plus_tree(const string& dir);

    //I/O operations functions 
    void insert(int key, const Record& rec);
    void remove(int key);
    vector<Record> rangeQueryR(int low, int high);

    //used to get root and handler info for main
    int getRootPage()  { return root_page; }
    page_handler& getHandler()  { return handler; }
    
    

    void exportToDot(const string& filename);

private:

    //b plus tree will have its own instance of handler
    page_handler handler;

    //stores the root page id 
    int root_page;

    //further explanation seen in cpp
    int createLeaf();
    int createInternal();

    void saveRoot();
    void insertRecursive(int pageID, int key, const Record& rec, int& promoted_key, int& new_child_page);

    void splitLeaf(leaf_node& node, const Record& rec, int& promoted_key, int& newPageID);
    void splitInternal(internal_node& node, int newKey, int new_child_page, int& promoted_key, int& newPageID);

    
    void removeRecursive(int pageID, int key, bool& merged);
    void printDotNode(ofstream& out, int pageID);
    
};

extern const string ROOT_META_FILE;
