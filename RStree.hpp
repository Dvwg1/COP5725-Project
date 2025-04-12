// --- RS-Tree ---

/*
References:
https://www.geeksforgeeks.org/cpp-program-to-implement-b-plus-tree/

https://github.com/andylamp/BPlusTree
https://github.com/myui/btree4j

Based off of our implementation of the Hilbert sorted B+-tree, we make modifications needed to 
transform it into the RS-tree described in the paper. An important note is that we will
not be using the optional insertion buffers, batch buffering, and queries will be done
using Hilbert values rather than with coordinates. Moreover, only the leaf nodes will be
stored in disk, as recommended by Dr. Zhao

---RS-Tree function and struct declarations ---

*/

//a check to make sure that this header file is only included once
#pragma once

//needed for data manipulation
#include <vector>
#include <string>
#include <iostream>

using namespace std;

//Note: pages represent a node

//sample buffer size. Testing: 16. Default: 256. Experimentals: 16, 256. 4096.
constexpr int SAMPLE_SIZE = 16;

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

//leaf node, stores records IN MEMORY
struct mem_leaf_node {

    //bool: 1 for leaf
    int is_leaf = 1;

    int record_num = 0;
    //int next_leaf_page = INVALID_PAGE;

    //max amount of records per leaf node
    Record records[MAX_LEAF_RECORDS];

    //initialize mem_leaf_node in nullptr
    mem_leaf_node* next_leaf = nullptr;
};

//leaf node, stores records ON DISK
struct disk_leaf_node {

    //bool: 1 for leaf
    int is_leaf = 1;

    int record_num = 0;
    int next_leaf_page = INVALID_PAGE;

    //max amount of records per leaf node
    Record records[MAX_LEAF_RECORDS];

};

//internal node, stores key (used as MBB) and children nodes
struct internal_node {

    //bool: 0 for internal
    int is_leaf = 0;
    int numKeys = 0;
  
    //children and keys
    int keys[MAX_INTERNAL_KEYS];
    void* children[MAX_INTERNAL_KEYS + 1];

    //sample buffer, stores Records in an array
    //its size can be up to 2s to account for merges, but can not have more than s records
    Record sample_buffer[2 * SAMPLE_SIZE];

    //determines if can have its sample_buffer filled or not
    bool sample_buffer_allowed = true;

    //count of sampled records
    int sample_count = 0;

};


//class used to handle pages for inserts, writes, reads. base of I/O functionality
//curcial for creating, using, and erasing leaf nodes on disk
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

    //full memory constructor
    b_plus_tree();

    //hybridized constructor - ie the one in use
    //our default sample size is set to 16 for le testing
    b_plus_tree(const string & directory_path);

    //same as in r-tree, but modified for memory applications
    void insert(int key, const Record& rec, bool build_mode);
    void remove(int key);
    vector<Record> rangeQuery(int low, int high);

    //visualizing
    void printTree();

    //used to get root and handler info for main
    int getRootPage()  { return root_page; }
    page_handler& getHandler()  { return handler; }

    //sampling related functions
    int getSubtreeRecordCount(void* node);

    void buildAllSamples();

  
    

    //void exportToDot(const string& filename);

private:

    //set at runtime
    int sample_size;

    /*translation helper functions*/

    //given pointer, returns corresponding page id
    inline int pointerToPageID(void* ptr) const;

    //given page id, returns corresponding pointer
    inline void* pageIDToPointer(int id) const;

    //verifier of pointer
    inline bool isPointerValid(void * ptr) const;

    /*memory related functionality*/
    //stores the root page id 
    void * root;
    void insertRecursive(void* node, int key, const Record& rec, int& promoted_key, void*& new_child, bool build_mode);

    //modified to be memory based
    void splitLeaf(mem_leaf_node* old_node, const Record& record, int& promoted_key, void*& new_node);
    void splitInternal(internal_node* old_node, int insert_key, void* insert_child, int& promoted_key, void*& new_node);

    void removeRecursive(void * node, int key, bool& merged, Record& deleted_record);

    
    /*disk related functionality*/
    //b plus tree will have its own instance of handler
    page_handler handler;

    //stores the root page id on disk
    int root_page;

    int createDiskLeaf();

    void splitDiskLeaf(disk_leaf_node & old_node, const Record & record, int & promoted_key, int & new_page_id);

    void saveRoot();

    //sampling related functions
    int recursiveNodeSubtreeCounter(void* node);

    vector<Record> BuildSamples(void* node, int d);

    vector<Record> sampleWithReplacement(const vector<Record> & record, int d);

    //used for standard inserts
    void updateSampleBuffer(internal_node* node, const Record& e);

    //removes sample instances
    void removeSample(internal_node* node, const Record& e);

    
};

extern const string ROOT_META_FILE;