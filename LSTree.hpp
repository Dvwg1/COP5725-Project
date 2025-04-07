//LSTree.hpp


#include "rtree.hpp"
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>



using namespace std; 


class ls_tree {
    public:
    //class for LSTree, collection of RTrees
    ls_tree(const string& dir); 

    vector<b_plus_tree> levels;

    private: 

} ;