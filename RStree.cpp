// --- Hilbert R-Tree ---

/*
References:
https://www.geeksforgeeks.org/cpp-program-to-implement-b-plus-tree/

https://github.com/andylamp/BPlusTree
https://github.com/myui/btree4j

Heavily modified implementation, with the first step being to transform a disk based 
r-tree into a memory based RS-tree, before modifying it to store leaves on disk.

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

using namespace std;

namespace fs = filesystem;

//root.meta is used for page directory organization
const string ROOT_META_FILE = "tree_pages/root.meta";


/*page_handler functions*/
/*
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
*/
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


b_plus_tree::b_plus_tree() {
    root = nullptr;
}

//used to insert new records into nodes 
void b_plus_tree::insert(int key, const Record& rec) {

    //if no root, initialize one
    if (!root) {
        
        //set root to leaf_node
        root = new leaf_node();
        reinterpret_cast<leaf_node*>(root)->is_leaf = 1;
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
void b_plus_tree::insertRecursive(void* node, int key, const Record& rec, int& promoted_key, void*& new_child) {

    //leaf node condition
    if (reinterpret_cast<leaf_node*>(node)->is_leaf) {

        //create a leaf node pointer instance
        leaf_node* leaf = reinterpret_cast<leaf_node*>(node);

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

//used when leaf node needs to be split
void b_plus_tree::splitLeaf(leaf_node* old_node, const Record& record, int& promoted_key, void*& new_node_ptr) {

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
    leaf_node* new_node = new leaf_node();
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

    //loops through all possible leaves
    while (!reinterpret_cast<leaf_node*>(node)->is_leaf) {

        //create instance of node
        internal_node* internal = reinterpret_cast<internal_node*>(node);

        //initialized iteration value
        int i = 0;

        //goes through the interal nodes to search for the right child
        while (i < internal->numKeys && low > internal->keys[i]) 
            i++;

        //sets node to the correct child node
        node = internal->children[i];
    }

    //create instance of leaf node
    leaf_node* leaf = reinterpret_cast<leaf_node*>(node);

    //at this point, should be a leaf
    while (leaf) {

        //records all of the record values
        for (int i = 0; i < leaf->record_num; ++i) {
            int key = leaf->records[i].hilbert;
            if (key > high) return result;
            if (key >= low) result.push_back(leaf->records[i]);
        }

        //goes to next leaf
        leaf = leaf->next_leaf;
    }

    //returns vector of records
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

    //leaf node condition
    if (reinterpret_cast<leaf_node*>(node)->is_leaf) {

        //creates a leaf node
        leaf_node* leaf = reinterpret_cast<leaf_node*>(node);

        //initial value
        int i = 0;

        //find the index of the key that is to be removed
        while (i < leaf->record_num && leaf->records[i].hilbert < key) 
            i++;
        
        //if the key exists, removed by shifting later records to the left
        if (i < leaf->record_num && leaf->records[i].hilbert == key) {

            for (int j = i; j < leaf->record_num - 1; ++j)
                leaf->records[j] = leaf->records[j + 1];

            //decrease the amount
            leaf->record_num--;
        }

        //deleting a leaf node should not trigger a merge
        merged = false;

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

        //leaf node condition
        if (reinterpret_cast<leaf_node*>(node)->is_leaf) {

            //create leaf node instance of the node
            leaf_node* leaf = reinterpret_cast<leaf_node*>(node);

            //print that it is, in fact, a leaf (wow)
            cout << "Leaf: ";
            
            //prints all of its records
            for (int i = 0; i < leaf->record_num; ++i){

                cout << leaf->records[i].hilbert << " ";
                /*
                cout << leaf->records[i].id << endl;
                cout << leaf->records[i].lat << endl;
                cout << leaf->records[i].lon << endl;
                cout << leaf->records[i].timestamp << endl; 
                */
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

            cout << "\n";

            //prints all of its children
            for (int i = 0; i <= internal->numKeys; ++i)
                q.push(internal->children[i]);
        }
    }
}

/*
void b_plus_tree::exportToDot(const string& filename) {
    ofstream out(filename);
    out << "digraph b_plus_tree {\nnode [shape=record];\n";
    queue<int> q;
    q.push(root_page);
    while (!q.empty()) {
        int pageID = q.front(); q.pop();
        printDotNode(out, pageID);
        char buffer[PAGE_SIZE];
        handler.readPage(pageID, buffer);
        int is_leaf;
        memcpy(&is_leaf, buffer, sizeof(int));
        if (!is_leaf) {
            internal_node* node = reinterpret_cast<internal_node*>(buffer);
            for (int i = 0; i <= node->numKeys; ++i) {
                out << "\"" << pageID << "\" -> \"" << node->children[i] << "\";\n";
                q.push(node->children[i]);
            }
        }
    }
    out << "}\n";
    out.close();
}

void b_plus_tree::printDotNode(ofstream& out, int pageID) {
    char buffer[PAGE_SIZE];
    handler.readPage(pageID, buffer);
    int is_leaf;
    memcpy(&is_leaf, buffer, sizeof(int));
    out << "\"" << pageID << "\" [label=\"";
    if (is_leaf) {
        leaf_node* node = reinterpret_cast<leaf_node*>(buffer);
        for (int i = 0; i < node->record_num; ++i) {
            out << node->records[i].hilbert;
            if (i != node->record_num - 1) out << "|";
        }
    } else {
        internal_node* node = reinterpret_cast<internal_node*>(buffer);
        for (int i = 0; i < node->numKeys; ++i) {
            out << "<f" << i << ">" << node->keys[i];
            if (i != node->numKeys - 1) out << "|";
        }
    }
    out << "\"];\n";
}
*/