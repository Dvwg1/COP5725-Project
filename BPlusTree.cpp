

/*This program is from geeksforgeeks and was
 updated to include the information for our 
 LS and RS trees based on Hilbert values.

 Reference: https://www.geeksforgeeks.org/cpp-program-to-implement-b-plus-tree/

*/

// C++ Program to Implement B+ Tree
#include <algorithm>
#include <iostream>
#include <vector>

//for parsing csv file
#include <fstream>
#include <sstream>
using namespace std;

// B plus tree class
template <typename T> class BPlusTree {
public:
    // structure to create a node
    struct Node {
        bool isLeaf;
        vector<T> keys;         //hilbert_keys
        vector<Node*> children;
        Node* next;

        vector<Node*> data;         //data rows(for leaf nodes)

        //variables for paper
        int id;
        float longitude;
        float latitude;
        int hilbert_value;

        // value of timestamp ??
        string timestamp; // or char*

        Node(bool leaf = false)
        : isLeaf(leaf), next(nullptr), id(0), latitude(0), longitude(0), timestamp(""), hilbert_value(0) {} //leave out csv values?


        Node(bool leaf, int ID, float lon, float lat, string ts, int hilbert)
            : isLeaf(leaf)
            , next(nullptr)
            //get values from csv file
            //id, lat, long, timestamp
            , id(ID)
            , longitude(lon)
            , latitude(lat)
            , timestamp(ts)
            , hilbert_value(hilbert)

        {
        }
    };

    Node* root;
    // Minimum degree (defines the range for the number of
    // keys)
    int t;

    // Function to split a child node
    void splitChild(Node* parent, int index, Node* child);

    // Function to insert a key in a non-full node
    void insertNonFull(Node* node, Node* newNode);

    // Function to remove a key from a node
    void remove(Node* node, Node* nodeRemoved);

    // Function to borrow a key from the previous sibling
    void borrowFromPrev(Node* node, int index);

    // Function to borrow a key from the next sibling
    void borrowFromNext(Node* node, int index);

    // Function to merge two nodes
    void merge(Node* node, int index);

    // Function to print the tree
    void printTree(Node* node, int level);

public:
    BPlusTree(int degree): root(nullptr), t(degree){}

    void insert(Node* node);
    bool search(Node* node);
    void remove(Node* node);
    vector<T> rangeQuery(T lower, T upper);
    void printTree();
};

// Implementation of splitChild function
template <typename T>
void BPlusTree<T>::splitChild(Node* parent, int index,
                              Node* child)
{
    Node* newChild = new Node(child->isLeaf);
    parent->children.insert(
        parent->children.begin() + index + 1, newChild);
    parent->keys.insert(parent->keys.begin() + index,
                        child->keys[t - 1]);

    newChild->keys.assign(child->keys.begin() + t,
                          child->keys.end());
    child->keys.resize(t - 1);

    if (!child->isLeaf) {
        newChild->children.assign(child->children.begin()
                                      + t,
                                  child->children.end());
        child->children.resize(t);
    }

    if (child->isLeaf) {
        newChild->next = child->next;
        child->next = newChild;
    }
}

// Implementation of insertNonFull function
template <typename T>
void BPlusTree<T>::insertNonFull(Node* node, Node* newNode)
{
    int key = newNode->hilbert_value;
    if (node->isLeaf) {
        node->keys.insert(upper_bound(node->keys.begin(),
                                      node->keys.end(),
                                      key),
                          key);
        //getting node->data ??
        //node->data.insert(node->data.begin() )
    }
    else {
        int i = node->keys.size() - 1;
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        if (node->children[i]->keys.size() == 2 * t - 1) {
            splitChild(node, i, node->children[i]);
            if (key > node->keys[i]) {
                i++;
            }
        }
        insertNonFull(node->children[i], newNode);
    }
}

// Implementation of remove function
template <typename T>
void BPlusTree<T>::remove(Node* node, Node* nodeRemoved)
{
    T key = nodeRemoved->hilbert_value; 
    // If node is a leaf
    if (node->isLeaf) {
        auto it = find(node->keys.begin(), node->keys.end(),
                       key);
        if (it != node->keys.end()) {
            node->keys.erase(it);
        }
    }
    else {
        int idx = lower_bound(node->keys.begin(),
                              node->keys.end(), key)
                  - node->keys.begin();
        if (idx < node->keys.size()
            && node->keys[idx] == key) {
            if (node->children[idx]->keys.size() >= t) {
                Node* predNode = node->children[idx];
                while (!predNode->isLeaf) {
                    predNode = predNode->children.back();
                }
                T pred = predNode->keys.back();
                node->keys[idx] = pred;
                remove(node->children[idx], predNode);
            }
            else if (node->children[idx + 1]->keys.size()
                     >= t) {
                Node* succNode = node->children[idx + 1];
                while (!succNode->isLeaf) {
                    succNode = succNode->children.front();
                }
                T succ = succNode->keys.front();
                node->keys[idx] = succ;
                remove(node->children[idx + 1], succNode);
            }
            else {
                merge(node, idx);
                remove(node->children[idx], nodeRemoved);
            }
        }
        else {
            if (node->children[idx]->keys.size() < t) {
                if (idx > 0
                    && node->children[idx - 1]->keys.size()
                           >= t) {
                    borrowFromPrev(node, idx);
                }
                else if (idx < node->children.size() - 1
                         && node->children[idx + 1]
                                    ->keys.size()
                                >= t) {
                    borrowFromNext(node, idx);
                }
                else {
                    if (idx < node->children.size() - 1) {
                        merge(node, idx);
                    }
                    else {
                        merge(node, idx - 1);
                    }
                }
            }
            remove(node->children[idx], nodeRemoved);
        }
    }
}

// Implementation of borrowFromPrev function
template <typename T>
void BPlusTree<T>::borrowFromPrev(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index - 1];

    child->keys.insert(child->keys.begin(),
                       node->keys[index - 1]);
    node->keys[index - 1] = sibling->keys.back();
    sibling->keys.pop_back();

    if (!child->isLeaf) {
        child->children.insert(child->children.begin(),
                               sibling->children.back());
        sibling->children.pop_back();
    }
}

// Implementation of borrowFromNext function
template <typename T>
void BPlusTree<T>::borrowFromNext(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index + 1];

    child->keys.push_back(node->keys[index]);
    node->keys[index] = sibling->keys.front();
    sibling->keys.erase(sibling->keys.begin());

    if (!child->isLeaf) {
        child->children.push_back(
            sibling->children.front());
        sibling->children.erase(sibling->children.begin());
    }
}

// Implementation of merge function
template <typename T>
void BPlusTree<T>::merge(Node* node, int index)
{
    Node* child = node->children[index];
    Node* sibling = node->children[index + 1];

    child->keys.push_back(node->keys[index]);
    child->keys.insert(child->keys.end(),
                       sibling->keys.begin(),
                       sibling->keys.end());
    if (!child->isLeaf) {
        child->children.insert(child->children.end(),
                               sibling->children.begin(),
                               sibling->children.end());
    }

    node->keys.erase(node->keys.begin() + index);
    node->children.erase(node->children.begin() + index
                         + 1);

    delete sibling;
}

// Implementation of printTree function
template <typename T>
void BPlusTree<T>::printTree(Node* node, int level)
{
    if (node != nullptr) {
        for (int i = 0; i < level; ++i) {
            cout << "  ";
        }
        for (const T& key : node->keys) {
            cout << key << " ";
        }
        cout << endl;
        for (Node* child : node->children) {
            printTree(child, level + 1);
        }
    }
}

// Implementation of printTree wrapper function
template <typename T> void BPlusTree<T>::printTree()
{
    printTree(root, 0);
}

// Implementation of search function
template <typename T> bool BPlusTree<T>::search(Node* node)
{
    T key = node ->hilbert_value; 
    Node* current = root;
    while (current != nullptr) {
        int i = 0;
        while (i < current->keys.size()
               && key > current->keys[i]) {
            i++;
        }
        if (i < current->keys.size()
            && key == current->keys[i]) {
            return true;
        }
        if (current->isLeaf) {
            return false;
        }
        current = current->children[i];
    }
    return false;
}

// Implementation of range query function
template <typename T>
vector<T> BPlusTree<T>::rangeQuery(T lower, T upper)
{
    vector<T> result;
    Node* current = root;
    while (!current->isLeaf) {
        int i = 0;
        while (i < current->keys.size()
               && lower > current->keys[i]) {
            i++;
        }
        current = current->children[i];
    }
    while (current != nullptr) {
        for (const T& key : current->keys) {
            if (key >= lower && key <= upper) {
                result.push_back(key);
            }
            if (key > upper) {
                return result;
            }
        }
        current = current->next;
    }
    return result;
}

// Implementation of insert function
template <typename T> void BPlusTree<T>::insert(Node* node)
{
    if (root == nullptr) {
        root = new Node(true);
        root->keys.push_back(node->hilbert_value);
        root->data.push_back(node); 

    }
    else {
        if (root->keys.size() == 2 * t - 1) {
            Node* newRoot = new Node();
            newRoot->children.push_back(root);
            splitChild(newRoot, 0, root);
            root = newRoot;
        }
        insertNonFull(root, node);
    }
}

// Implementation of remove function
template <typename T> void BPlusTree<T>::remove(Node* node)
{
    if (root == nullptr) {
        return;
    }
    remove(root, node);
    if (root->keys.empty() && !root->isLeaf) {
        Node* tmp = root;
        root = root->children[0];
        delete tmp;
    }
}



__uint128_t convertID(string ID) {

    __uint128_t ret = 0;
    for (char c : ID) {
        ret *= 16;
        if (c >= '0' && c <='9') {
            ret += c - '0';
        } 
        else if (c >= 'a' && c <= 'f') {
            ret += c - 'a' + 10;
        }
        else if (c >= 'A' && c <= 'F') {
            ret += c - 'A' + 10;
        }

    }
    return ret;
}



// Main function to test the B+ Tree implementation
int main()
{
    BPlusTree<int> tree(3);

    //reading from csv file
    //vector<BPlusTree<int>::Node*> nodes;

    ifstream file("newCSVforDC.csv");
    if (!file.is_open()) {
        cout << "Error opening the file \n";
        return 0;
    }

    string line;
    getline(file, line); 
    while(getline(file, line)) { 
        stringstream ss(line);
        string getID, getLong, getLat, getTS, getH;

        if(getline(ss, getID, ',') && 
            getline(ss,getLong, ',') && 
            getline(ss,getLat, ',') && 
            getline(ss,getTS, ',') && 
            getline(ss,getH, ',') ) {
                //cout << "id, lat, lon, ts, hilbert:" << getID << "\n" ; 
                //unsigned long long id = stoull(getID, nullptr,16);
                __uint128_t id = convertID(getID);
                float lat = stof(getLat);
                float lon = stof(getLong);
                int hilbert = stoi(getH);

                BPlusTree<int>::Node* node = new BPlusTree<int>::Node( false, id, lon, lat, getTS, hilbert); 
                //nodes.push_back(node);
                tree.insert(node);
            }
    }

    file.close();

    
    

    // cout << "B+ Tree after insertions:" << endl;
    // tree.printTree();

    // // Search for a key
    // int searchKey = 15;
    // cout << "\nSearching for key " << searchKey << ": "
    //      << (tree.search(searchKey) ? "Found" : "Not Found")
    //      << endl;

    // // Perform a range query
    // int lower = 10, upper = 25;
    // vector<int> rangeResult = tree.rangeQuery(lower, upper);
    // cout << "\nRange query [" << lower << ", " << upper
    //      << "]: ";
    // for (int key : rangeResult) {
    //     cout << key << " ";
    // }
    // cout << endl;

    // // Remove a key
    // int removeKey = 20;
    // tree.remove(removeKey);
    // cout << "\nB+ Tree after removing " << removeKey << ":"
    //      << endl;
     tree.printTree();

    return 0;
}
