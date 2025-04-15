//base_model_lstree.cpp


#include "rtree.hpp"
#include "LSTree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <cstdlib>

//for count()
#include <bits/stdc++.h>

using namespace std; 


//debugging function
//used to confirm that the children are actually child page IDs
void print_children( b_plus_tree& tree) {

    //creates a queue to hold nodes, adds the root page for searching
    queue<int> node_queue;
    node_queue.push(tree.getRootPage());

    //buffer initialize
    char buffer[PAGE_SIZE];

    //loops until the entire queue has been iterated
    while (!node_queue.empty()) {

        //set page id to the first node, and remove
        int page_id = node_queue.front();
        node_queue.pop();

        //use the id to read its value
        tree.getHandler().readPage(page_id, buffer);

        //get the leaf/internal status
        int is_leaf;
        memcpy(&is_leaf, buffer, sizeof(int));

        //ends loop if not leaf
        if (is_leaf) 
            continue;

        //brings out the node, and prints its keys and children
        internal_node* node = reinterpret_cast<internal_node*>(buffer);
        cout << "Internal Node Page ID: " << page_id << endl;

        cout << "  Keys: ";
        for (int i = 0; i < node->numKeys; ++i)
            cout << node->keys[i] << " ";
            
        cout << "\n  Children: ";
        for (int i = 0; i <= node->numKeys; ++i)
            cout << node->children[i] << " ";
        cout << "\n\n";

        for (int i = 0; i <= node->numKeys; ++i)
            node_queue.push(node->children[i]);
    }
}


int main() {

    srand(time(0));
    string inputFile;
    cout << "Enter CSV file path: ";
    getline(cin, inputFile);

    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        return 1;
    }

    //string numRecord;
    //cout << "Enter the number of records in this file: ";
    //getline(cin, numRecord);
    //divided by 4 to start since each num has 50% chance to be entered in LSTree
    int numRecords = 0;

    //letting user know index construction experiment has begun
    cout << "Index construction cost experiment beginning now: " << endl ;


    //starts the timer, at this point the file should have been found
	auto startBuild = chrono::high_resolution_clock::now();

    ls_tree tree("ls_tree_pages");
    max_min_hilbert treesMaxMin;
    treesMaxMin.max_hilbert = 0;
    treesMaxMin.min_hilbert = INT32_MAX;  

    string line;

    int counter = 0;
    int directoryCounter = 0;
    const char LSTreeDir[20] = "ls_tree_pages/btree"; 
    string directroy = LSTreeDir + to_string(directoryCounter); 
    string strDCounter; 
    getline(file, line); // Skip header

    
    
    while (getline(file, line)) {

        stringstream ss(line);
        string idStr, latStr, lonStr, tsStr, hStr;

        numRecords++; 
        
        //numRecords = numRecords / 2; 

        if (getline(ss, idStr, ',') &&
            getline(ss, latStr, ',') &&
            getline(ss, lonStr, ',') &&
            getline(ss, tsStr, ',') &&
            getline(ss, hStr, ',')) {

            try {
                Record r;
                strncpy(r.id, idStr.c_str(), sizeof(r.id));
                r.id[sizeof(r.id) - 1] = '\0';
                //cout << "test1" << endl;
                r.lon = stof(lonStr);
                r.lat = stof(latStr);
                strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                r.hilbert = stoi(hStr);

                if(r.hilbert > treesMaxMin.max_hilbert) {treesMaxMin.max_hilbert = r.hilbert; } 
                if(r.hilbert < treesMaxMin.min_hilbert) {treesMaxMin.min_hilbert = r.hilbert; } 
                

                /* THIS IS NOT DONE UNTIL SECOND TREE
                if (counter > numRecords) {
                    //new to create a new btree
                    numRecords = numRecords / 2;
                    //cout << "numRecords: "<< numRecords << endl; 
                    counter = 0; 
                    directoryCounter++;
                    strDCounter = to_string(directoryCounter);
                    directroy = LSTreeDir + strDCounter; 
                    tree.maxMin.push_back(treesMaxMin); 
                    treesMaxMin.max_hilbert = r.hilbert;
                    treesMaxMin.min_hilbert = r.hilbert; 

                } 
                if (numRecords <= 6) { //will be 256000
                    //cout << "numRecords: "<< numRecords << endl;
                    break; 
                }*/ 
                tree.addToTree(directroy, r.hilbert, r);
                
                
            }
            
            //error catching for debugging
            catch (const exception& e) {
                cerr << "Invalid line (skipping): " << line << "\\n";
                cerr << "  Error: " << e.what() << "\\n";
            }
        } 
    }

    //ONCE WHOLE FILE IS READ, CREATE SMALLER TREES FROM SAMPLING (COIN-FLIP)
    //int numRecords = counter; 
    cout << "num of records in csv: " << numRecords << endl; 
    int totalNumRecords = numRecords; 
    numRecords = numRecords / 2; 
    directoryCounter++;
    strDCounter = to_string(directoryCounter);
    directroy = LSTreeDir + strDCounter; 
    tree.maxMin.push_back(treesMaxMin); 

    //get back to top of file
    //https://stackoverflow.com/questions/5343173/returning-to-beginning-of-file-after-getline
    file.clear();
    file.seekg(0); 
    vector<string> prev_records; 
    getline(file, line); // Skip header    
    

    //loops and continues to read file until 256000 in last tree
    while (numRecords > 256000) {
        counter = 0;
        file.clear();           
        file.seekg(0);          
        getline(file, line); // Skip header
    
        while (getline(file, line)) {
            stringstream ss(line);
            string idStr, latStr, lonStr, tsStr, hStr;
    
            if (getline(ss, idStr, ',') &&
                getline(ss, latStr, ',') &&
                getline(ss, lonStr, ',') &&
                getline(ss, tsStr, ',') &&
                getline(ss, hStr, ',')) {
    
                try {
                    Record r;
                    strncpy(r.id, idStr.c_str(), sizeof(r.id));
                    r.id[sizeof(r.id) - 1] = '\0';
                    r.lon = stof(lonStr);
                    r.lat = stof(latStr);
                    strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                    r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                    r.hilbert = stoi(hStr);
    
                    if (directroy != "ls_tree_pages/btree1") {
                        if (!count(prev_records.begin(), prev_records.end(), r.id)) continue;
                    }
    
                    if (rand() % 2 == 1) { continue; } 
    
                    tree.addToTree(directroy, r.hilbert, r);
                    prev_records.push_back(r.id);
                    counter++;
    
                    if (r.hilbert > treesMaxMin.max_hilbert) treesMaxMin.max_hilbert = r.hilbert;
                    if (r.hilbert < treesMaxMin.min_hilbert) treesMaxMin.min_hilbert = r.hilbert;
    
                    if (counter >= numRecords) 
                        break;
    
                } catch (const exception& e) {
                    cerr << "Invalid line (skipping): " << line << "\n";
                    cerr << "  Error: " << e.what() << "\n";
                }
            }
        }
    
        // Finished one level
        tree.maxMin.push_back(treesMaxMin);
        //reset values
        treesMaxMin.max_hilbert = 0;
        treesMaxMin.min_hilbert = INT32_MAX; 
    
        numRecords /= 2;
        directoryCounter++;
        strDCounter = to_string(directoryCounter);
        directroy = LSTreeDir + strDCounter;
    }
    


    tree.insertMemoryTree(directroy);
    

    //ends timer after sorted data set is complete, calculates elapsed time
	auto endBuild = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_timeBuild = endBuild - startBuild;
    cout << "Tree built from CSV and stored on disk, except last tree in memory.\n";
	cout << "Index Construction Cost: " << total_timeBuild.count() << " seconds" << endl;



    for (size_t i = 0; i < tree.maxMin.size(); i++)
    {
        cout << "tree " << i<< " has min_hilbert of: " << tree.maxMin[i].min_hilbert << endl; 

        cout << "tree " << i<< " has max_hilbert of: " << tree.maxMin[i].max_hilbert << endl; 
    }
    

    int experimentInput = -1;

    do {
        cout << endl << endl << "Experiments to run: " << endl;
        cout << "1. Query cost, vary k (# of samples): " << endl;
        cout << "2. Query cost, vary q (# of elements in range query): " << endl;
        cout << "EXPERIMENT 3 ONLY FOR RS-TREE " << endl;
        cout << "4. Update cost (FYI this experiment will end program and tree will have to be built again): " << endl;
        cout << "5. Exit program" << endl; 
        cout << "Please enter the experiment to run: ";
        cin >> experimentInput; 
        if (experimentInput == 5) 
            break; 
        else if (experimentInput == 4) {
            cout << "Update Cost experiment now beginning" << endl ;
            cout << "Now inserting 5000 records into LS-Tree" << endl; 
            counter = 0; 
            //insert 5000 records into tree
            file.clear();
            file.seekg(0);
            getline(file, line); //skip header
            auto startInsert = chrono::high_resolution_clock::now();
            while (counter < 5000) {       //5000 insertions
                    getline(file,line);
                    stringstream ss(line);
                    string idStr, latStr, lonStr, tsStr, hStr;
                    if (getline(ss, idStr, ',') &&
                    getline(ss, latStr, ',') &&
                    getline(ss, lonStr, ',') &&
                    getline(ss, tsStr, ',') &&
                    getline(ss, hStr, ',')) {
        
                        try {
                            Record r;
                            strncpy(r.id, idStr.c_str(), sizeof(r.id));
                            r.id[sizeof(r.id) - 1] = '\0';
                            r.lon = stof(lonStr);
                            r.lat = stof(latStr);
                            strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                            r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                            r.hilbert = stoi(hStr);
            
                            // if (directroy != "ls_tree_pages/btree1") {
                            //     if (!count(prev_records.begin(), prev_records.end(), r.id)) continue;
                            // }
            
                            //if (rand() % 2 == 1) { continue; } 
            
                            tree.insertMoreRecords(r);
                            //prev_records.push_back(r.id);
                            counter++;
            
                            // if (r.hilbert > treesMaxMin.max_hilbert) treesMaxMin.max_hilbert = r.hilbert;
                            // if (r.hilbert < treesMaxMin.min_hilbert) treesMaxMin.min_hilbert = r.hilbert;
            
                            //if (counter >= numRecords) break;
            
                        } catch (const exception& e) {
                            cerr << "Invalid line (skipping): " << line << "\n";
                            cerr << "  Error: " << e.what() << "\n";
                        }
                    }
                //}
                
            }
            //ends timer after sorted data set is complete, calculates elapsed time
            auto endInsert = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_timeInsert = endInsert - startInsert;
            cout << "Tree built from CSV and stored on disk, except last tree in memory.\n";
            cout << "Insertion Cost: " << total_timeInsert.count() << " seconds" << endl;

            return 0; 

            //now after inserting those records, they need to be removed. 
            cout << endl << "Now Cost of Deletion will be recorded. " << endl; 
            auto startDeletion = std::chrono::high_resolution_clock::now();
            counter = 0; 

            while (counter < 2) {       //5000 deletions
                getline(file,line);
                stringstream ss(line);
                string idStr, latStr, lonStr, tsStr, hStr;
                if (getline(ss, idStr, ',') &&
                getline(ss, latStr, ',') &&
                getline(ss, lonStr, ',') &&
                getline(ss, tsStr, ',') &&
                getline(ss, hStr, ',')) {
    
                    try {
                        Record r;
                        strncpy(r.id, idStr.c_str(), sizeof(r.id));
                        r.id[sizeof(r.id) - 1] = '\0';
                        r.lon = stof(lonStr);
                        r.lat = stof(latStr);
                        strncpy(r.timestamp, tsStr.c_str(), sizeof(r.timestamp));
                        r.timestamp[sizeof(r.timestamp) - 1] = '\0';
                        r.hilbert = stoi(hStr);
                        cout << "removing id: " << r.id << " and hilbert: " << r.hilbert << endl; 
        
                        // if (directroy != "ls_tree_pages/btree1") {
                        //     if (!count(prev_records.begin(), prev_records.end(), r.id)) continue;
                        // }
        
                        //if (rand() % 2 == 1) { continue; } 
        
                        tree.removeHilbert(r);
                        //prev_records.push_back(r.id);
                        counter++;
        
                        // if (r.hilbert > treesMaxMin.max_hilbert) treesMaxMin.max_hilbert = r.hilbert;
                        // if (r.hilbert < treesMaxMin.min_hilbert) treesMaxMin.min_hilbert = r.hilbert;
        
                        //if (counter >= numRecords) break;
        
                    } catch (const exception& e) {
                        cerr << "Invalid line (skipping): " << line << "\n";
                        cerr << "  Error: " << e.what() << "\n";
                    }
                }
            //}
            
        }


            auto endDeletion = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_timeDeletion = endDeletion - startDeletion;
            cout << "Tree built from CSV and stored on disk, except last tree in memory.\n";
            cout << "Deletion Cost: " << total_timeDeletion.count() << " seconds" << endl;




            

        }
        else if (experimentInput == 1) {
            //query cost vary k 
            int minInput, maxInput, kInput; 
            float kFloatInp; 
            cout << "Query cost, vary k experiment" << endl;
            
            cout << "Please enter the (space separated) min and max hilbert values to search: " ;
            cin >> minInput >> maxInput; 
            cout << "Now please enter k (0.02, 0.04, 0.06, 0.08, 0.1): " ;
            cin >> kFloatInp; 
            kInput = totalNumRecords * kFloatInp; 
            cout << kInput << endl; 
             
            //cout << minInput << maxInput << kInput;
            //return 0;

            auto startQueryK = chrono::high_resolution_clock::now();
            vector<Record> results = tree.querying(9849, 9857, kInput); 
            cout << "getting out of querying" << endl; 
            for (size_t i = 0; i < results.size(); i++)
            {
                cout << "results ID: " << results[i].id
                << ", Lat: " << results[i].lat
                << ", Lon: " << results[i].lon
                << ", Time: " << results[i].timestamp
                << ", Hilbert: " << results[i].hilbert << endl; 
                
            }
            
            //ends timer after sorted data set is complete, calculates elapsed time
            auto endQueryK = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_time = endQueryK - startQueryK;
            cout << "RangeQuery Cost: " << total_time.count() << " seconds" << endl;

        } else if (experimentInput == 2) {
            //query cost vary q
            int minInput, maxInput, kInput;
            kInput = 5; //5000 or 10000
            //float kFloatInp; 
            cout << "Query cost, vary q experiment" << endl;
            cout << "In this experiment, k is set to 5000 and 10000 respectively" << endl;
            
            cout << "Please enter the (space separated) min and max hilbert values to search: " ;
            cin >> minInput >> maxInput; 
            //cout << "Now please enter k (0.02, 0.04, 0.06, 0.08, 0.1): " ;
            //cin >> kFloatInp; 
            cout << "Starting experiment... " << endl; 

            auto startQueryQ = chrono::high_resolution_clock::now();
            vector<Record> results = tree.querying(minInput, maxInput, kInput); 

            /*for (size_t i = 0; i < results.size(); i++)
                {
                    cout << "results ID: " << results[i].id
                    << ", Lat: " << results[i].lat
                    << ", Lon: " << results[i].lon
                    << ", Time: " << results[i].timestamp
                    << ", Hilbert: " << results[i].hilbert << endl; 
                    
                }*/
            
            //ends timer after sorted data set is complete, calculates elapsed time
            auto endQueryQ = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_time = endQueryQ - startQueryQ;
            cout << "RangeQuery Cost: " << total_time.count() << " seconds" << endl;
            
        } else { 
            cout << "Please enter one of the experiments below.  " << endl;
        }

    } while(experimentInput != 4) ;

    //children debugging function call
    //print_children(btree);

    //tree.exportToDot("tree.dot");
    //cout << "DOT file generated: tree.dot\n";
    file.close();
    return 0;
}