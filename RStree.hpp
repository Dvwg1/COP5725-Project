// --- RS-Tree ---

/*
References:
https://www.geeksforgeeks.org/cpp-program-to-implement-b-plus-tree/

https://www.mikeash.com/pyblog/friday-qa-2012-07-27-lets-build-tagged-pointers.html

https://stackoverflow.com/questions/33131800/c-generating-random-numbers-in-a-loop-using-default-random-engine
https://en.cppreference.com/w/cpp/numeric/random/binomial_distribution

https://github.com/andylamp/BPlusTree
https://github.com/myui/btree4j

Heavily modified implementation, with the first step being to transform a disk based 
r-tree into a memory based RS-tree, before modifying it to store leaves on disk.

based on the r-tree that was in turn based on [1]

[2] as well as the slides served as the inspiration for how the pointer to id and id to pointer system works,
through the use of tagged pointers

[5][6] are used as inspiration for the disk based implementation of the B+ tree.

[3][4] are used as reference for random number generation

Ultimately, this RS-tree is based on the RS-tree proposed and implemented by Wang et al., with some
modifications and changes

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
constexpr int SAMPLE_SIZE = 256;
//extern int SAMPLE_SIZE;

//defines the Page size to be 8000 bytes, as Wang et al. have their page sizes set to 8 KB
constexpr size_t PAGE_SIZE = 8000;

//Arbritary amount of records to be included in a node, set to 100 to ensure no overflow
constexpr int MAX_LEAF_RECORDS = 100;

//max fanout of 16, so there is to be 0-15 keys per node
constexpr int MAX_INTERNAL_KEYS = 15;

//a -1 page will be used as an invalid check
constexpr long int INVALID_PAGE = -1;

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

    //used to mark if a record has been reported/rejected
    //by default, none have been reported/rejected yet
    bool disabled = false;
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
    long int next_leaf_page = INVALID_PAGE;

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
//crucial for creating, using, and erasing leaf nodes on disk
class page_handler {

public:

    //constructor
    page_handler(const string& dir);

    //gets the next page
    int pageIncrementer();
    
    //disk read/write operations
    void writePage(long int pageID, const void* data, size_t dataSize);
    void readPage(long int pageID, void* buffer);

    //gets the page path for further operations
    string getPagePath(long int pageID);

private:
    //variables
    string directory;
    long int next_page_id;

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
    vector<Record> rangeQuery(int low, int high, long int );

    //visualizing
    void printTree();

    //used to get root and handler info for main
    long int getRootPage()  { return root_page; }
    page_handler& getHandler()  { return handler; }

    //sampling related functions
    long int getSubtreeRecordCount(void* node);

    long int getNonDisabledSubtreeRecordCount(void* node);

    void buildAllSamples();

    //Wang et al based query function
    vector<Record> SampleFirstRS(int low, int high, size_t k);


private:

    //set at runtime
    int sample_size;

    /*translation helper functions*/

    //given pointer, returns corresponding page id./s
    inline long int pointerToPageID(void* ptr) const;

    //given page id, returns corresponding pointer
    inline void* pageIDToPointer(long int id) const;

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
    long int root_page;

    int createDiskLeaf();

    void splitDiskLeaf(disk_leaf_node & old_node, const Record & record, int & promoted_key, long int & new_page_id);

    void saveRoot();

    //sampling related functions
    long int recursiveNodeSubtreeCounter(void* node);

    long int recursiveNonDisabledSubtreeCounter(void* node);

    vector<Record> BuildSamples(void* node, int d);

    vector<Record> sampleWithReplacement(const vector<Record> & record, int d);

    //used for standard inserts
    void updateSampleBuffer(internal_node* node, const Record& e);

    //removes sample instances
    void removeSample(internal_node* node, const Record& e);

    //replenishes buffers recursively
    void replenishSamples(internal_node* node);

    //re-enables all disabled records
    void reenableAllRecordsInSubtree(void* node);
    
};

extern const string ROOT_META_FILE;