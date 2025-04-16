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

--- Hilbert R-Tree function and helper function implementation ---

*/


#include "rtree.hpp"

//data manipulation
#include <fstream>
#include <iostream>
#include <sstream>

//for dot 
#include <queue>

//misc
#include <cstring>
#include <filesystem>
#include <algorithm>
#include <cassert>

using namespace std;

namespace fs = filesystem;

//root.meta is used for page directory organization
const string ROOT_META_FILE = "tree_pages/root.meta";


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

//used to save the root page id for recals
void b_plus_tree::saveRoot() {

    //opens root.meta file for reading
    ofstream out(ROOT_META_FILE);

    //reads in the value
    out << root_page;
}

//used to create a leaf node
int b_plus_tree::createLeaf() {

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
    leaf_node* node = reinterpret_cast<leaf_node*>(buffer);
    *node = leaf_node{};

    //calls writePage to create a new node with the right size, buffer, and pid
    handler.writePage(pid, buffer, sizeof(leaf_node));

    //returns id for linking
    return pid;

}

//used to create an internal node
int b_plus_tree::createInternal() {
   
    //gets the page id of what next page is supposed to be
    int pid = handler.pageIncrementer();

    //buffer initialized to 0 to prevent memory leaks
    char buffer[PAGE_SIZE] = {0};

    //initialize node in buffer
    internal_node* node = reinterpret_cast<internal_node*>(buffer);
    *node = internal_node{};

    //children experimental
    node->is_leaf = 0;

    //calls writePage to create a new node with the right size, buffer, and pid
    handler.writePage(pid, buffer, sizeof(internal_node));

    //returns id for linking
    return pid;
}

//used to insert new records into nodes 
void b_plus_tree::insert(int key, const Record& rec) {

    //initialize to -1 to indicate no splitting has occured
    int promoted_key = -1, new_child_page = -1;

    //calls the recursive interstion function, which will update
    //promoted_key and new_child_page as needed
    insertRecursive(root_page, key, rec, promoted_key, new_child_page);

    //if this occurs, the root is to be split which indicates need a new root node
    if (new_child_page != -1) {

        //debugging code
        //cout << "entered the new root check" << endl;

        int new_root = createInternal();

        //memory safe root node intiilialization
        char buffer[PAGE_SIZE] = {0};
        internal_node* root = reinterpret_cast<internal_node*>(buffer);
        *root = internal_node{};

        //root node values, current has just one key, set key to promoted key, has the old root as its child
        root->is_leaf = 0;
        root->numKeys = 1;
        root->keys[0] = promoted_key;
        root->children[0] = root_page;
        root->children[1] = new_child_page;
        
        //write root node, save root node 
        handler.writePage(new_root, buffer, sizeof(internal_node));
        root_page = new_root;
        saveRoot();
    }

}

//used for record insertion, splitting, and promoted key upward propagation
void b_plus_tree::insertRecursive(int pageID, int key, const Record& rec, int& promoted_key, int& new_child_page) {

    //memory safe buffer intialization and read
    char buffer[PAGE_SIZE] = {0};  
    handler.readPage(pageID, buffer);

    //used to determine, from buffer, if the page is for an internal or leaf node
    int is_leaf;
    memcpy(&is_leaf, buffer, sizeof(int));

    //leaf node condition
    if (is_leaf) {

        //create a leaf node using the buffer
        leaf_node* node = reinterpret_cast<leaf_node*>(buffer);

        //if the leaf node/page has enough room for a record
        if (node->record_num < MAX_LEAF_RECORDS) {

            //used to maintain hilbert sort order
            int i = node->record_num - 1;
            while (i >= 0 && node->records[i].hilbert > key) {
                node->records[i + 1] = node->records[i];
                i--;
            }
            
            //updates records
            node->records[i + 1] = rec;
            node->record_num++;

            //creates a page with needed values
            handler.writePage(pageID, buffer, sizeof(leaf_node));

            //update root related values
            promoted_key = -1;
            new_child_page = -1;
        } 
        
        //if leaf node/page has NO room for a record
        else {

            //calls the fucntion to split the leaf node, and create new page leaf
            splitLeaf(*node, rec, promoted_key, new_child_page);
            handler.writePage(pageID, buffer, sizeof(leaf_node));
        }

    } 
    
    //internal node condition
    else {

        //creates internal node using buffer
        internal_node* node = reinterpret_cast<internal_node*>(buffer);

        //used to maintain key order 
        int i = 0;
        while (i < node->numKeys && key > node->keys[i]) i++;
        int child = node->children[i];

        //temporary promoted key and new child page to be inserted recursively
        int temp_promote_key, temp_new_child_page;
        insertRecursive(child, key, rec, temp_promote_key, temp_new_child_page);

        //if the new child page is valid 
        if (temp_new_child_page != -1) {

            //if the internal node has enough room for a child
            if (node->numKeys < MAX_INTERNAL_KEYS) {

                for (int j = node->numKeys; j > i; --j) {
                    node->keys[j] = node->keys[j - 1];
                    node->children[j + 1] = node->children[j];
                }

                //updates node information
                node->keys[i] = temp_promote_key;
                node->children[i + 1] =temp_new_child_page;
                node->numKeys++;

                //creates page
                handler.writePage(pageID, buffer, sizeof(internal_node));

                //updates promoted key and new child page accordingly
                promoted_key = -1;
                new_child_page = -1;
            } 

            //if internal node can have no more children
            else {

                //calls the fucntion to split the internal node, and create new page internal
                splitInternal(*node, temp_promote_key,temp_new_child_page, promoted_key, new_child_page);
                handler.writePage(pageID, buffer, sizeof(internal_node));
            }
        } 
        
        //if no other conditions are met, set values to invalid
        else {
            promoted_key = -1;
            new_child_page = -1;
        }
    }
}

//used when leaf node needs to be split
void b_plus_tree::splitLeaf(leaf_node& old_node, const Record& record, int& promoted_key, int& new_page_id) {

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
    leaf_node* new_node = reinterpret_cast<leaf_node*>(buffer);
    new_node->record_num = record_total- split_index;
    memcpy(new_node->records, temp + split_index, new_node->record_num * sizeof(Record));

    //sets leaf condition to 1 and relinks to old node
    new_node->is_leaf = 1;
    new_node->next_leaf_page = old_node.next_leaf_page;

    //creates and relinks new page id
    new_page_id = createLeaf(); 
    old_node.next_leaf_page = new_page_id;

    //creates a page with the new node information
    handler.writePage(new_page_id, buffer, sizeof(leaf_node));

    //promotes first key 
    promoted_key = new_node->records[0].hilbert;

    /* //debugging
    cout << "Splitting leaf. Promoted key: " << promoted_key
              << ", New leaf page ID: " << new_page_id << endl;
    */
}

//used when internal node needs to be split
void b_plus_tree::splitInternal(internal_node& old_node, int insert_key, int insert_page_id, int& promoted_key, int& new_page_id) {


    //creates temporary array to hold all node keys and children, in addition to one more 
    const int totalKeys = old_node.numKeys;
    int keys[MAX_INTERNAL_KEYS + 1];
    int children[MAX_INTERNAL_KEYS + 2];

    //loop to maintain key order, similar to how record order is maintained
    //i will record where new key is to go
    int i = 0;
    while (i < totalKeys && insert_key > old_node.keys[i]) 
        i++;

    //shifts keys to the right of i and inserts the new key correctly
    for (int j = 0; j < i; ++j) 
        keys[j] = old_node.keys[j];

    keys[i] = insert_key;

    for (int j = i; j < totalKeys; ++j) 
        keys[j + 1] = old_node.keys[j];

    //shifts keys to the right of i and inserts new child correctly
    for (int j = 0; j <= i; ++j) 
        children[j] = old_node.children[j];

    children[i + 1] = insert_page_id;
    
    for (int j = i + 1; j <= totalKeys; ++j)
        children[j + 1] = old_node.children[j];

    //total to indicate key total, which is used to determine how many records to split left
    int key_total= totalKeys + 1;
    int mid =key_total/ 2;

    //promotes middle key
    promoted_key = keys[mid];

    //rebuilds old node with left half
    old_node.numKeys = mid;
    for (int j = 0; j < mid; ++j) 
        old_node.keys[j] = keys[j];

    for (int j = 0; j <= mid; ++j) 
        old_node.children[j] = children[j];

    //creates new node, and fills the right half
    char buffer[PAGE_SIZE] = {};
    internal_node* new_node = reinterpret_cast<internal_node*>(buffer);

    //experimental value
    new_node->is_leaf = 0; 

    //update the original node, keeping the first mid keys and corresponding child pointers
    new_node->numKeys = key_total- mid - 1;
    for (int j = 0; j < new_node->numKeys; ++j) 
        new_node->keys[j] = keys[mid + 1 + j];

    for (int j = 0; j <= new_node->numKeys; ++j) 
        new_node->children[j] = children[mid + 1 + j];

    // Step 6: Allocate and write new internal node
    new_page_id = createInternal();  // logs and allocates
    handler.writePage(new_page_id, buffer, sizeof(internal_node));

  
}

//range query implementation
vector<Record> b_plus_tree::rangeQueryR(int low, int high) {

    //stores the results matching the query
    vector<Record> result;

    //buffer to store page info
    char buffer[PAGE_SIZE];

    //sets the current node to root
    int current_node= root_page;

    while (true) {

        //reads in pages into the buffer
        handler.readPage(current_node, buffer);

        //ends the infinite loop once a leaf is found after copying its information
        int is_leaf;
        memcpy(&is_leaf, buffer, sizeof(int));
        if (is_leaf) 
            break;

        //if we haven't gotten to the above check then node is internal
        internal_node* node = reinterpret_cast<internal_node*>(buffer);

        //goes to the next node
        int i = 0;
        while (i < node->numKeys && low > node->keys[i]) 
            i++;
        current_node= node->children[i];
    }

    //at this point, node should be a leaf node
    while (current_node != INVALID_PAGE) {

        //read in page values
        handler.readPage(current_node, buffer);
        leaf_node* node = reinterpret_cast<leaf_node*>(buffer);

        //records all of the record values
        for (int i = 0; i < node->record_num; ++i) {
            int key = node->records[i].hilbert;
            if (key > high) return result;
            if (key >= low) result.push_back(node->records[i]);
        }

        //goes to neighboring leaf
        current_node = node->next_leaf_page;
    }

    //returns vector of records 
    return result;
}

//calls the remove recursive function, while setting merged status to false
void  b_plus_tree::removeR(int key) {

    bool merged = false;
    removeRecursive(root_page, key, merged);
}

//main remove functionality, as it is down recursively
void  b_plus_tree::removeRecursive(int pageID, int key, bool& merged) {

    //memory safe buffer intialization and read
    char buffer[PAGE_SIZE] = {0};  
    handler.readPage(pageID, buffer);

    //used to determine, from buffer, if the page is for an internal or leaf node
    int is_leaf;
    memcpy(&is_leaf, buffer, sizeof(int));

    //leaf node condition
    if (is_leaf) {

        //create a leaf node using the buffer
        leaf_node* node = reinterpret_cast<leaf_node*>(buffer);
        
        //initial value
        int i = 0;

        //find the index of the key that is to be removed 
        while (i < node->record_num && node->records[i].hilbert < key) 
            i++;

        //if the key exists, removed by shifrting later records to the left
        if (i < node->record_num && node->records[i].hilbert == key) {

            for (int j = i; j < node->record_num - 1; ++j)
                node->records[j] = node->records[j + 1];

            //decrease the count 
            node->record_num--;

            //write the updated leaf node back to disk
            handler.writePage(pageID, buffer, sizeof(leaf_node));
        }

        //deleting a leaf node should not trigger a merge
        merged = false;
    } 
    
    //internal node condition
    else {

        //create an internal node using the buffer
        internal_node* node = reinterpret_cast<internal_node*>(buffer);

        //initial value
        int i = 0;

        //find the index of key that is to be removed
        while (i < node->numKeys && key > node->keys[i]) 
            i++;

        //recurse into the child that may contain 
        bool childMerged = false;
        removeRecursive(node->children[i], key, childMerged);

        //if the child indicated that it was merged, was empited
        if (childMerged && i + 1 < node->numKeys) {

            //remove the corresponding key and child pointer in the internal node
            for (int j = i; j < node->numKeys - 1; ++j) {

                node->keys[j] = node->keys[j + 1];
                node->children[j + 1] = node->children[j + 2];
            }

            //decrease the number of keys
            node->numKeys--;

            //write the updated node back to the disk
            handler.writePage(pageID, buffer, sizeof(internal_node));
        }

        //marked as merged
        merged = (node->numKeys == 0);
    }
}

