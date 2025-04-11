// --- Hilbert R-Tree ---

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

the 2nd served as the inspiration for how the pointer to id and id to pointer system works,
through the use of tagged pointers

The last two links are used as inspiration for the disk based implementation of the B+ tree.

--- RS-Tree function and helper function implementation ---

*/


#include "RStree.hpp"

//data manipulation
#include <fstream>
#include <iostream>
#include <sstream>

//for memory storage 
#include <queue>

//misc
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <cassert>

//sampling
#include <chrono>
#include <random>

using namespace std;

namespace fs = filesystem;

//root.meta is used for page directory organization
const string ROOT_META_FILE = "RStree_pages/root.meta";


/*page_handler functions*/
//constructor, calls the cre
page_handler::page_handler(const string& dir) : directory(dir),next_page_id(0) {

    //used to create the directory
    fs::create_directories(directory);
    loadNextPageID();
}

//used for proper page numbering and management. Each time a page is added, the next page variable
//is incremented to ensure sequential ordering.
void page_handler::loadNextPageID() {
   
    //iterates through the directory
    for (fs::directory_iterator it = fs::directory_iterator(directory); it != fs::directory_iterator(); ++it) 
    {
        const auto& f = *it;

        //sets name to current page
        string name = f.path().filename().string();

        //finds page instance
        if (name.rfind("page_", 0) == 0) {

            //extracts the number from the page, converts to int
            int id = stoi(name.substr(5));

            //sets next page id to extract id + 1
            next_page_id = max(next_page_id, id + 1);
        }
    }
}

//provides the next page id, plus 1
int page_handler::pageIncrementer() {

    return next_page_id++;
}

//crucial piece of code: writes data (including records or keys) to pages
void page_handler::writePage(int pageID, const void* data, size_t dataSize) {

    //opens the page to be written to
    ofstream out(getPagePath(pageID), ios::binary);

    //initialize the buffer, with 800 byte capacity, to 0 to prevent memory leaks
    char buffer[PAGE_SIZE] = {};  

    //safely copies data into the buffer
    memcpy(buffer, data, dataSize);

    //writes to page
    out.write(buffer, PAGE_SIZE);

}

//used to read pages into memory
void page_handler::readPage(int pageID, void* buffer) {
    
    //opens page to be read from
    ifstream in(getPagePath(pageID), ios::binary);

    //reads in the page
    in.read(reinterpret_cast<char*>(buffer), PAGE_SIZE);
}

//provides path to page, crucial for search operations
string page_handler::getPagePath(int pageID) {

    //returns the page needed in directory format
    return directory + "/page_" + to_string(pageID) + ".bin";
}

/*end of page handler functions*/

/*B Plus Tree function declaration*/
/*
//constructor
b_plus_tree::b_plus_tree(const string& dir) : handler(dir) {

    //opens the root.meta file for writing
    ifstream in(ROOT_META_FILE);

    //if exists and real, will read root page ID into root_page
    if (in.is_open()) {
        in >> root_page;
    } 
    
    //if doesn't exist yet, create a leaf node, save to disk, saves root value
    else {
        root_page = createLeaf();
        saveRoot();
    }
}
*/

//used to save the root page id for recals
/*
void b_plus_tree::saveRoot() {

    //opens root.meta file for reading
    ofstream out(ROOT_META_FILE);

    //reads in the value
    out << root_page;
}
*/

//full memory constructor
// perhaps the sketchiest patch I have ever done here at the great FSU
// with a non-sense directory for handler
/*b_plus_tree::b_plus_tree() : handler("dont_open_nothing_inside/"){
    root = nullptr;
}*/

//hybrid constructor
b_plus_tree::b_plus_tree(const string& dir) : handler(dir) {

    //creates the specified directory, from directory loads in next page ID
    int leaf_page_id = handler.pageIncrementer();

    //buffer initialized safely
    char buffer[PAGE_SIZE] = {};

    //buffer intialized as disk leaf node
    disk_leaf_node* disk_leaf = reinterpret_cast<disk_leaf_node*>(buffer);
    
    //initialize all fields to default
    * disk_leaf = disk_leaf_node{};
    disk_leaf->is_leaf = 1;
    disk_leaf->record_num =0;
    //initialized to be linked to nothing, since would be first record
    disk_leaf->next_leaf_page = INVALID_PAGE;

    //write node to disk
    handler.writePage(leaf_page_id, buffer, sizeof(disk_leaf_node));

    //root created as internal node in memory
    internal_node * root_internal = new internal_node();

    //initialize memory values
    root_internal->is_leaf = 0;
    root_internal->numKeys = 0;

    //root made to point to leaf using the translator
    root_internal->children[0] = pageIDToPointer(leaf_page_id);

    //tree root set to newly created root
    root = root_internal;

}

//used to create a leaf node
int b_plus_tree::createDiskLeaf() {

    /* //debugging code
    static int leaf_page_count = 0;
    ++leaf_page_count;
    cout << "created leaf page #" << leaf_page_count << endl;
    */

    //gets the page id of what next page is supposed to be
    int pid = handler.pageIncrementer();

    //buffer initialized to 0 to prevent memory leaks
    char buffer[PAGE_SIZE] = {0};

    //initialize node in buffer
    disk_leaf_node* node = reinterpret_cast<disk_leaf_node*>(buffer);
    *node = disk_leaf_node{};

    //calls writePage to create a new node with the right size, buffer, and pid
    handler.writePage(pid, buffer, sizeof(disk_leaf_node));

    //returns id for linking
    return pid;

}


//given a page_id, converts it to an address, and then
//uses OR on it with 0x8.. , in order to produce a 
//basic encoded address, that will be used as pointer to the leaf
inline void* b_plus_tree::pageIDToPointer(int id) const {
    
    return reinterpret_cast<void*>((static_cast<uintptr_t>(id) | 0x8000000000000000));
}

//given a pointer, will remove the 0x8.. using 0x7FFF.., and translates the
//encoded address value back to a page_id, which will 
//correspond to a leaf_node 
inline int b_plus_tree::pointerToPageID(void* ptr) const {

    return static_cast<int>(reinterpret_cast<uintptr_t>(ptr) & 0x7FFFFFFFFFFFFFFF);
}

//verfication
inline bool b_plus_tree::isPointerValid(void* ptr) const {

    return (reinterpret_cast<uintptr_t>(ptr) & 0x8000000000000000) != 0;
}

//used to insert new records into nodes 
void b_plus_tree::insert(int key, const Record& rec) {

    //if no root, initialize one
    if (!root) {
        
        //set root to mem_leaf_node
        /*root = new mem_leaf_node();
        reinterpret_cast<mem_leaf_node*>(root)->is_leaf = 1;*/

        //create a disk leaf page
        int page_id = createDiskLeaf();

        //create root as an internal memory node
        internal_node* new_root = new internal_node();
        new_root->is_leaf = 0;
        new_root->numKeys = 0;
        new_root->children[0] = pageIDToPointer(page_id);

        root = new_root;
    
    }

    //promoted key will move up upon update
    int promoted_key = -1;

    //pointer to newly creatd node
    void* new_child = nullptr;

    //calls recursive
    insertRecursive(root, key, rec, promoted_key, new_child);

    //if root splits, create new internal, set root to new_root
    if (new_child) {
        
        //internal node instance, to serve as the root
        internal_node* new_root = new internal_node();

        //values initialized for the new root node
        new_root->is_leaf = 0;
        new_root->numKeys = 1;
        new_root->keys[0] = promoted_key;
        new_root->children[0] = root;
        new_root->children[1] = new_child;

        //sets root node
        root = new_root;
    }
}

//used for record insertion, splitting, and promoted key upward propagation
//hybridized version

void b_plus_tree::insertRecursive(void* node, int key, const Record& rec, int& promoted_key, void*& new_child) {

    //checks the node to see if it translates to a tagged pointer
    if (isPointerValid(node)) {

        //loads in the page_id from the pointer
        int page_id = pointerToPageID(node);

        //buffer size allocation
        char buffer[PAGE_SIZE];

        //read in values from page
        handler.readPage(page_id, buffer);

        //creates an instance of disk node
        disk_leaf_node* leaf = reinterpret_cast<disk_leaf_node*>(buffer);

        //if node has capacity for more records
        if (leaf->record_num < MAX_LEAF_RECORDS) {

            //inserts while keeping the Hilbert sort order
            int i = leaf->record_num - 1;
            while (i >= 0 && leaf->records[i].hilbert > key) {

                leaf->records[i + 1] = leaf->records[i];
                i--;
            }
            leaf->records[i + 1] = rec;
            leaf->record_num++;

            //writes records back to the leaf node
            handler.writePage(page_id, buffer, sizeof(disk_leaf_node));

            //sets promote_key to -1
            promoted_key = -1;
            new_child = nullptr;

        }

        //if node at eh max capacity
        else{

            //creates instance of new id
            int new_page_id;

            //calls the split function to split the records
            splitDiskLeaf(*leaf, rec, promoted_key, new_page_id);

            //writes record
            handler.writePage(page_id, buffer, sizeof(disk_leaf_node));

            //gets new pointer value, from the page
            new_child = pageIDToPointer(new_page_id);

        }

        return;

    }

    //internal node condition
    else {

        //creates internal node using buffer
        internal_node* internal = reinterpret_cast<internal_node*>(node);

        //used to main key order
        int i = 0;
        while (i < internal->numKeys && key > internal->keys[i]) i++;
        void* child = internal->children[i];

        //temporary pormoted key and new child to be inserted recursively
        int temp_key = -1;
        void* temp_child = nullptr;
        insertRecursive(child, key, rec, temp_key, temp_child);

        //if the new child page is valid
        if (temp_child) {

            //if the internal node has enough room for a child
            if (internal->numKeys < MAX_INTERNAL_KEYS) {

                //assignments
                for (int j = internal->numKeys; j > i; --j) {
                    internal->keys[j] = internal->keys[j - 1];
                    internal->children[j + 1] = internal->children[j];
                }

                //updates node information
                internal->keys[i] = temp_key;
                internal->children[i + 1] = temp_child;
                internal->numKeys++;

                //updates promoted key and new child page accordingly
                promoted_key = -1;
                new_child = nullptr;


            } 
            
            //if internal node can have no more children
            else {

                //calls the function to split the internal node, and create new page internal
                splitInternal(internal, temp_key, temp_child, promoted_key, new_child);
            }

        } 
        
        //if no other conditions are met, set values to invalid
        else {
            promoted_key = -1;
            new_child = nullptr;
        }
    }

}

//used for record insertion, splitting, and promoted key upward propagation
/*
void b_plus_tree::insertRecursive(void* node, int key, const Record& rec, int& promoted_key, void*& new_child) {

    //determine node type
    bool is_mem_leaf = reinterpret_cast<mem_leaf_node*>(node)->is_leaf;

    //leaf node condition
    if (reinterpret_cast<mem_leaf_node*>(node)->is_leaf) {

        //create a leaf node pointer instance
        mem_leaf_node* leaf = reinterpret_cast<mem_leaf_node*>(node);

        //if the leaf node/page has enough room for a record
        if (leaf->record_num < MAX_LEAF_RECORDS) {

            //used to maintain hilber sort order
            int i = leaf->record_num - 1;
            while (i >= 0 && leaf->records[i].hilbert > key) {
                leaf->records[i + 1] = leaf->records[i];
                i--;
            }

            //updates records
            leaf->records[i + 1] = rec;
            leaf->record_num++;

            //updates root related values
            promoted_key = -1;
            new_child = nullptr;

        }

        //if leaf node/page has NO room for a record
         else {

            //calls the function to split and create a new leaf node
            splitLeaf(leaf, rec, promoted_key, new_child);
        }
    } 
    
    //internal node condition
    else {

        //creates internal node using buffer
        internal_node* internal = reinterpret_cast<internal_node*>(node);

        //used to main key order
        int i = 0;
        while (i < internal->numKeys && key > internal->keys[i]) i++;
        void* child = internal->children[i];

        //temporary pormoted key and new child to be inserted recursively
        int temp_key = -1;
        void* temp_child = nullptr;
        insertRecursive(child, key, rec, temp_key, temp_child);

        //if the new child page is valid
        if (temp_child) {

            //if the internal node has enough room for a child
            if (internal->numKeys < MAX_INTERNAL_KEYS) {

                //assignments
                for (int j = internal->numKeys; j > i; --j) {
                    internal->keys[j] = internal->keys[j - 1];
                    internal->children[j + 1] = internal->children[j];
                }

                //updates node information
                internal->keys[i] = temp_key;
                internal->children[i + 1] = temp_child;
                internal->numKeys++;

                //updates promoted key and new child page accordingly
                promoted_key = -1;
                new_child = nullptr;

            } 
            
            //if internal node can have no more children
            else {

                //calls the function to split the internal node, and create new page internal
                splitInternal(internal, temp_key, temp_child, promoted_key, new_child);
            }

        } 
        
        //if no other conditions are met, set values to invalid
        else {
            promoted_key = -1;
            new_child = nullptr;
        }
    }
}
*/

//disk version of leaf split, based on logic from r-tree
void b_plus_tree::splitDiskLeaf(disk_leaf_node & old_node, const Record & record, int & promoted_key, int & new_page_id){


    //creates temporary array to hold all node records in addition to one more 
    Record temp[MAX_LEAF_RECORDS + 1];
    int i = 0, j = 0;

    //going off of the hilbert value, ensure record inserted in correct sorted order
    while (i < old_node.record_num && old_node.records[i].hilbert < record.hilbert)
        temp[j++] = old_node.records[i++];

    temp[j++] = record;

    while (i < old_node.record_num)
        temp[j++] = old_node.records[i++];

    //total to indicate record total, which is used to determine how many records to split left
    int record_total= j;
    int split_index = record_total/ 2;

    //fills in the old node 
    old_node.record_num = split_index;
    memcpy(old_node.records, temp, split_index * sizeof(Record));

    //in a memory safe way, creates and initializes new node
    char buffer[PAGE_SIZE] = {0};
    disk_leaf_node* new_node = reinterpret_cast<disk_leaf_node*>(buffer);
    new_node->record_num = record_total- split_index;
    memcpy(new_node->records, temp + split_index, new_node->record_num * sizeof(Record));

    //sets leaf condition to 1 and relinks to old node
    new_node->is_leaf = 1;
    new_node->next_leaf_page = old_node.next_leaf_page;

    //creates and relinks new page id
    new_page_id = createDiskLeaf(); 
    old_node.next_leaf_page = new_page_id;

    //creates a page with the new node information
    handler.writePage(new_page_id, buffer, sizeof(disk_leaf_node));

    //promotes first key 
    promoted_key = new_node->records[0].hilbert;

    /* //debugging
    cout << "Splitting leaf. Promoted key: " << promoted_key
              << ", New leaf page ID: " << new_page_id << endl;
    */
}


//used when leaf node needs to be split
void b_plus_tree::splitLeaf(mem_leaf_node* old_node, const Record& record, int& promoted_key, void*& new_node_ptr) {

    //creates temporary array to hold all node records in addition to one more
    Record temp[MAX_LEAF_RECORDS + 1];
    int i = 0, j = 0;

    //going off of hilbert value, ensure record inserted in correct sorted order
    while (i < old_node->record_num && old_node->records[i].hilbert < record.hilbert)
        temp[j++] = old_node->records[i++];

    //ensure insertion in the right location
    temp[j++] = record;

    //copies the rest of the records after the insertion
    while (i < old_node->record_num)
        temp[j++] = old_node->records[i++];

    //used to determing how many records to split left
    int mid = (MAX_LEAF_RECORDS + 1) / 2;

    //fills in the old node, using a for loop
    old_node->record_num = mid;
    for (i = 0; i < mid; ++i)
        old_node->records[i] = temp[i];

    //creates and initiazlies the new node, pulling from temp using the for loop
    mem_leaf_node* new_node = new mem_leaf_node();
    new_node->record_num = j - mid;
    for (i = 0; i < new_node->record_num; ++i)
        new_node->records[i] = temp[mid + i];

    //sets leaf condition to 1 and relinks to old node
    new_node->is_leaf = 1;
    new_node->next_leaf = old_node->next_leaf;

    //relinks new node
    old_node->next_leaf = new_node;

    //promotes first key and sets new node pointer to the new node
    promoted_key = new_node->records[0].hilbert;
    new_node_ptr = new_node;
}

//used when internal node needs to be split
void b_plus_tree::splitInternal(internal_node* old_node, int insert_key, void* insert_child, int& promoted_key, void*& new_node_ptr) {

    //creates temporary array to hold all node keys and children, in addition to one more
    int keys[MAX_INTERNAL_KEYS + 1];
    void* children[MAX_INTERNAL_KEYS + 2];

    //loop to maintain key order, similar to how record order is maintained
    //i will record where new key is to go
    int i = 0;
    while (i < old_node->numKeys && insert_key > old_node->keys[i]) 
        i++;

    //shifts keys to the right of i and inserts the new key correctly
    for (int j = 0; j < i; ++j)
        keys[j] = old_node->keys[j];

    //shifts keys to the right of 1 and inserts the new key correctly
    for (int j = i; j < old_node->numKeys; ++j)
        keys[j + 1] = old_node->keys[j];

    keys[i] = insert_key;

    //copies all child pointers up to and including child, insert new child pointer, fix remaining
    for (int j = 0; j <= i; ++j)
        children[j] = old_node->children[j];

    children[i + 1] = insert_child;

    for (int j = i + 1; j <= old_node->numKeys; ++j)
        children[j + 1] = old_node->children[j];

    //midpoint calculation, and key promotion based on it
    int mid = (MAX_INTERNAL_KEYS + 1) / 2;
    promoted_key = keys[mid];

    //updates left node 
    old_node->numKeys = mid;
    for (int j = 0; j < mid; ++j)
        old_node->keys[j] = keys[j];

    //copies corresponding children for the left nde
    for (int j = 0; j <= mid; ++j)
        old_node->children[j] = children[j];

    //create new right-hand internal node
    internal_node* new_node = new internal_node();
    new_node->numKeys = MAX_INTERNAL_KEYS - mid;

    //copies keys into new node
    for (int j = 0; j < new_node->numKeys; ++j)
        new_node->keys[j] = keys[mid + 1 + j];

    //copies children into new node
    for (int j = 0; j <= new_node->numKeys; ++j)
        new_node->children[j] = children[mid + 1 + j];

    //sets new node pointer to new node
    new_node_ptr = new_node;
}

//range query implementation
std::vector<Record> b_plus_tree::rangeQuery(int low, int high) {

    //stores the results matching the queries
    std::vector<Record> result;

    //if no root, return that
    if (!root) 
        return result;

    //initializes starting node to root
    void* node = root;

    //takes in non-leaf, ie internal nodes 
    while (!isPointerValid(node)) {

        //create internal node instance
        internal_node* internal = reinterpret_cast<internal_node*>(node);

        //gets all children
        int i = 0;
        while (i < internal->numKeys && low > internal->keys[i])
            i++;
        
        //add children to the node children array
        node = internal->children[i];
    }

    //if out of while loop, can assume node is a leaf and extract its page_id
    int page_id = pointerToPageID(node);

    //loops through all internals until -1
    while (page_id != INVALID_PAGE) {

        //buffer size creation
        char buffer[PAGE_SIZE];

        //reads in node, create leaf node
        handler.readPage(page_id, buffer);
        disk_leaf_node* leaf = reinterpret_cast<disk_leaf_node*>(buffer);


        //goes through records
        for (int i = 0; i < leaf->record_num; ++i) {

            //puts or returns record in questio
            int key = leaf->records[i].hilbert;
            if (key > high) 
                return result;
            if (key >= low) 
                result.push_back(leaf->records[i]);
        }

        //begins next leaf page
        page_id = leaf->next_leaf_page; 
    }

    return result;

}

//calls the remove recursive function, while setting merged status to false
void b_plus_tree::remove(int key) {

    //error checking addition
    if (!root) 
        return;

    //same logic as in r-tree
    bool merged = false;
    removeRecursive(root, key, merged);
}

//main remove functionality, as it is down recursively
void b_plus_tree::removeRecursive(void* node, int key, bool& merged) {

    //if pointer is to a leaf, leaf node condition
    if (isPointerValid(node)) {

        //gets the page id from the leaf
        int page_id = pointerToPageID(node);

        cout << "Attempting to remove " << key << " from page " << page_id << endl;

        //creates buffer
        char buffer[PAGE_SIZE];
        
        //reads in information from leaf
        handler.readPage(page_id, buffer);

        //create instance of leaf node
        disk_leaf_node * leaf = reinterpret_cast<disk_leaf_node*>(buffer);

        //find the index of the key that is to be removed
        int i = 0;
        while (i < leaf->record_num && leaf->records[i].hilbert < key)
            i++;

        //if the key exists, removed by shifting later records to the left
        if (i < leaf->record_num && leaf->records[i].hilbert == key) {

            //shift records left to remove
            for (int j = i; j < leaf->record_num - 1; j++)
                leaf->records[j] = leaf->records[j + 1];
            
            leaf->record_num--;

            //writes page back to disk
            handler.writePage(page_id, buffer, sizeof(disk_leaf_node));

        }

         //debug
         cout << "Removed record with hilbert: " << key << " from page " << page_id << endl;


        //deleting a leaf node should not trigger a merge
        merged = false;
        return;
     
    } 
    
    //internal node condition
    else {


        //create an internal node
        internal_node* internal = reinterpret_cast<internal_node*>(node);

        //initial value
        int i = 0;

        //find the index of the key that is to be removed
        while (i < internal->numKeys && key > internal->keys[i]) 
            i++;

        //recurse into the child that may contain
        bool child_merged = false;
        removeRecursive(internal->children[i], key, child_merged);

        //if the child indicated that is was merged, was empitied
        if (child_merged && i + 1 < internal->numKeys) {
            
            //remove the corresponding key and child pointer in the internal node
            for (int j = i; j < internal->numKeys - 1; ++j) {

                internal->keys[j] = internal->keys[j + 1];
                internal->children[j + 1] = internal->children[j + 2];
            }

            //decrease the number of keys
            internal->numKeys--;
        }

        //marked as merged
        merged = (internal->numKeys == 0);
    }
}


/*sampling implementation*/

//public function to get the number of records in a node's subtree (children, grandchildren, etc)
int b_plus_tree::getSubtreeRecordCount(void* node){

    //empty tree error check
    if (!root) {

        cout << "Tree is empty.\n";
        return 1;
    }

    //calls actual counter, and returns total counter
    int total_records = recursiveNodeSubtreeCounter(node);

    return total_records;
    
    
}

//recursively gets the count of all records in a node's subtree
int b_plus_tree::recursiveNodeSubtreeCounter(void* node) {

    //get direct count if leaf condition
    if (isPointerValid(node)) {

        //gets the page id, allocates buffer, reads from page
        int page_id = pointerToPageID(node);
        char buffer[PAGE_SIZE];
        handler.readPage(page_id, buffer);

        //creates instance of disk leaf to return record_num
        disk_leaf_node * leaf = reinterpret_cast<disk_leaf_node*>(buffer);
        return leaf->record_num;
    }

    //internal condition, creates instance from node
    internal_node * internal = reinterpret_cast<internal_node*>(node);

    int total_records =0;

    //loops through all of its children to get count
    for (int i = 0; i <= internal->numKeys; i++) {

        total_records += recursiveNodeSubtreeCounter(internal->children[i]);
    }

   
    return total_records;

}


//adapted from the pseudocode algorithm provided by Wang et al., used to generate samples for internal nodes
vector<Record> b_plus_tree::BuildSamples(void* node, int d){

    //leaf node condition
    if (isPointerValid(node)){
        
        //gets the page id, allocates buffer, reads from page
        int page_id = pointerToPageID(node);
        char buffer[PAGE_SIZE];
        handler.readPage(page_id, buffer);

        //creates instance of disk leaf to return record_num
        disk_leaf_node * leaf = reinterpret_cast<disk_leaf_node*>(buffer);

        //creates vector to hold all of leaf records
        vector<Record> records;

        //populatres vector from leaf record
        for (int i =0; i < leaf->record_num; i++){
            //mfers call me the sandwich
            records.push_back(leaf->records[i]);
        }

        //return random samples
        return sampleWithReplacement(records, d);
    }

    //internal node condition, creates instance from provided node
    internal_node* internal = reinterpret_cast<internal_node*>(node);

    //similar to above record vector, but this time is for subtree
    //basically: S <- 
    vector<Record> subtree_records;

    //checks amount of samples
    if (internal->sample_count < SAMPLE_SIZE ) {

        d = d + (2 * SAMPLE_SIZE) - internal->sample_count;
    }

    //need lines 6-10

    //internal node condition
    //loops through all of its children to get count
    for (int i = 0; i <= internal->numKeys; i++) {


    }





}


//created with inspiration from the description of sampling from Wang et al.
//takes in records, returns d number of samples
vector<Record> b_plus_tree::sampleWithReplacement(const vector<Record> & record, int d){

    //will hold samples
    vector<Record> samples;

    //edge case for d=0, just returns no samples from empty vector
    if (record.empty())
        return samples;

    //defines the default random engine used for random sampling, seeded with computer clock  
    //references: https://stackoverflow.com/questions/33131800/c-generating-random-numbers-in-a-loop-using-default-random-engine
    static default_random_engine rand_gen (chrono::steady_clock::now().time_since_epoch().count());

    //uses predefined binomial distribution function
    //references: https://en.cppreference.com/w/cpp/numeric/random/binomial_distribution
    binomial_distribution<int> dist(record.size() -1, .5);

    //loops through the passed in records, sampling at random and pushing into samples vector
    //uses d as the constraint
    for (int i = 0; i < d; i++ ){

        int round = dist(rand_gen);
        round = min(round, static_cast<int>((record.size() -1)));
        samples.push_back(record[round]);

    }

    return samples;
}


//used to print the tree, more so for our error checking tbh
void b_plus_tree::printTree() {

    //error checking (empty tree)
    if (!root) {
        cout << "Tree is empty.\n";
        return;
    }

    //queue to hold the tree
    queue<void*> q;

    //add the root
    q.push(root);

    //iterates until all nodes have been iterated through
    while (!q.empty()) {

        //create a variable with the node from the front of the queue, remove it
        void* node = q.front(); 
        q.pop();

        //leaf condition, going oof of the tagged pointer logic
        if (isPointerValid(node)) {

            //gets the page_id
            int page_id = pointerToPageID(node);

            //buffer creation
            char buffer[PAGE_SIZE];
            handler.readPage(page_id, buffer);
            disk_leaf_node * leaf = reinterpret_cast<disk_leaf_node*>(buffer);

            //print all of the hilbert values and id's from the records
            cout << "Disk Leaf [Page " << page_id << "]: ";
            for (int i =0; i < leaf->record_num; i++) {

                cout << leaf->records[i].hilbert << " ";
            }
            cout << "\n";

        }

        //internal node condition
        else {

            //create internal node instance of the node
            internal_node* internal = reinterpret_cast<internal_node*>(node);

            //prints that, believe it or not, it is an internal (extra wow)
            cout << "Internal: ";

            //prints all of its keys
            for (int i = 0; i < internal->numKeys; ++i)
                cout << internal->keys[i] << " ";

            int total_records = getSubtreeRecordCount(internal);

            cout << "total records: " << total_records << endl;
            
            cout << "\n";

            //prints all of its children
            for (int i = 0; i <= internal->numKeys; ++i)
                q.push(internal->children[i]);
        }
    }
}
