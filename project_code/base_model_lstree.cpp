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



int main() {

    srand(time(0));
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();

    //get input file
    string inputFile;
    cout << "Enter CSV file path: ";
    getline(cin, inputFile);

    //check if file can be opened
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
    treesMaxMin.min_hilbert = INT64_MAX;

    string line;

    //int counter = 0;
    int directoryCounter = 0;
    const char LSTreeDir[20] = "ls_tree_pages/btree"; 
    string directroy = LSTreeDir + to_string(directoryCounter); 
    string strDCounter; 
    getline(file, line); // Skip header

    
    //read data set from file
    while (getline(file, line)) {

        stringstream ss(line);
        string idStr, latStr, lonStr, tsStr, hStr;

        //counter to get number of records added to first tree
        numRecords++; 

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
    //cout << "finished first tree" << endl; 
    int totalNumRecords = numRecords; 
    // numRecords = numRecords / 2; 
    // directoryCounter++;
    // strDCounter = to_string(directoryCounter);
    // directroy = LSTreeDir + strDCounter; 
    tree.maxMin.push_back(treesMaxMin); 

    //get back to top of file
    //https://stackoverflow.com/questions/5343173/returning-to-beginning-of-file-after-getline
    //file.clear();
    //file.seekg(0); 
    vector<string> prev_records; 
    //getline(file, line); // Skip header    
    
    //cout << "starting trees after btree0 " << endl; 

    //build the next trees
    //continues until less than 256K nodes (Page 7, Wang et al.)

    while(numRecords > 256000) {
        numRecords = numRecords / 2; 
        directoryCounter++;
        strDCounter = to_string(directoryCounter);
        directroy = LSTreeDir + strDCounter; 
        //tree.maxMin.push_back(treesMaxMin); 
        //cout << "Inserting to " << directroy << endl; 

        b_plus_tree& prevTree = tree.levels.rbegin()->second;
        vector<Record> prevTreeRecords = tree.getRecords(prevTree) ;
        for (size_t i = 0; i < prevTreeRecords.size(); i++)
        {
            if (rand() % 2 == 1) {
                //insert into next tree
                tree.addToTree(directroy, prevTreeRecords[i].hilbert, prevTreeRecords[i]);
                

            } 
        }
        tree.maxMin.push_back(treesMaxMin); 
    }
    

    
    //cout << "finished making trees and now inserting memoryTree" << endl; 


    tree.insertMemoryTree(directroy);

    //cout << "completed insertMemoryTree" << endl; 
    

    //ends timer after sorted data set is complete, calculates elapsed time
	auto endBuild = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_timeBuild = endBuild - startBuild;
    cout << "Tree built from CSV and stored on disk, except last tree in memory.\n";
	cout << "Index Construction Cost: " << total_timeBuild.count() << " seconds" << endl;


    //give user max and min of LS-Tree
    cout << "LS-Tree " << " has min_hilbert of: " << tree.maxMin[0].min_hilbert << endl; 

    cout << "LS-Tree " << " has max_hilbert of: " << tree.maxMin[0].max_hilbert << endl;



    //menu to select what experiment to do next
    int experimentInput = -1;

    do {
        cout << endl << endl << "Experiments to run: " << endl;
        cout << "1. Query cost, vary k (# of samples): " << endl;
        cout << "2. Query cost, vary q (# of elements in range query): " << endl;
        cout << "EXPERIMENT 3 ONLY FOR RS-TREE " << endl;
        cout << "4. Update cost (FYI this experiment will end program and tree will have to be built again): " << endl;
        cout << "5. Exit program" << endl; 
        cout << "Please enter the experiment to run: ";
        while (!(cin >> experimentInput)) {
            cout << "Invalid input. Please enter an integer: ";
        }
        if (experimentInput == 5) 
            break; 
        else if (experimentInput == 4) {
            //UPDATE COST EXPERIMENT (INSERTIONS AND DELETIONS)
            cout << "Update Cost experiment now beginning" << endl ;
            cout << "Now inserting 5000 records into LS-Tree" << endl; 
            //counter = 0; 
            //insert 5000 records into tree
            // file.clear();
            // file.seekg(0);
            // getline(file, line); //skip header
            

            b_plus_tree& firstTree = tree.levels.begin()->second;
            //get records then shuffle, then insert 500 
            vector<Record> firstTreeRecords = tree.getRecords(firstTree) ;
            shuffle (firstTreeRecords.begin(), firstTreeRecords.end(), default_random_engine(seed));

            //capture time for every 50 insertions
            vector<double> insertTimes; 
            int increments = 50; 
            auto startInsert = chrono::high_resolution_clock::now();
            for (int i = 0; i < 5001; i++)
            {
                    //insert into next tree
                    //tree.addToTree(directroy, firstTreeRecords[i].hilbert, firstTreeRecords[i]);
                    if (i == increments) {
                        auto endIncrement = std::chrono::high_resolution_clock::now();
                        chrono::duration<double> total_timeIncrement = endIncrement - startInsert;
                        insertTimes.push_back(total_timeIncrement.count()); 
                        if (increments % 1000 == 0) //print out when 1000s
                            cout << "increment at " << i << " is " << total_timeIncrement.count() << endl ;
                        increments += 50; 
                    }
                    tree.insertMoreRecords(firstTreeRecords[i]); 
                    totalNumRecords++;
                    //cout << "record just inserted: " << firstTreeRecords[i].id << endl; 
            }

            //cout << "num records after insertion: " << totalNumRecords << endl; 

            //ends timer after sorted data set is complete, calculates elapsed time
            //auto endInsert = std::chrono::high_resolution_clock::now();
            //chrono::duration<double> total_timeInsert = endInsert - startInsert;
            cout << "Insertion Cost experiment over. " << endl;

            vector<Record> treeAfterInsert = tree.getRecords(tree.levels.begin()->second);
            cout << "num records after insertion: " << treeAfterInsert.size() << endl; 

            //give yourself a lil break between inserts and deletes 
            sleep(2); 


            //now after inserting those records, they need to be removed. 
            cout << endl << "Now Cost of Deletion will be recorded. " << endl; 
    
            vector<double> deletionsTimes; 
            //reset increments for deletions
            increments = 50; 
            auto startDeletion = std::chrono::high_resolution_clock::now();
            for (int i = 0; i < 5001; i++)
            {
                    //insert into next tree
                    //tree.addToTree(directroy, firstTreeRecords[i].hilbert, firstTreeRecords[i]);
                    //tree.insertMoreRecords(firstTreeRecords[i]); 
                if (i == increments) {
                    auto endIncrement = std::chrono::high_resolution_clock::now();
                    chrono::duration<double> total_timeIncrement = endIncrement - startDeletion;
                    deletionsTimes.push_back(total_timeIncrement.count()); 
                    if (increments % 1000 == 0) //print out when 1000s
                        cout << "increment at " << i << " is " << total_timeIncrement.count() << endl ;
                    increments += 50; 
                }
                tree.removeHilbert(firstTreeRecords[i]); 
                totalNumRecords--;
            }

            vector<Record> treeAfterDelete = tree.getRecords(tree.levels.begin()->second);
            cout << "num records after deletion: " << totalNumRecords << endl; 


            //auto endDeletion = std::chrono::high_resolution_clock::now();
            //chrono::duration<double> total_timeDeletion = endDeletion - startDeletion;
            cout << "Deletion Cost experiment over. " << endl;
            

        }
        else if (experimentInput == 1) {
            //query cost vary k 
            int minInput = -1;
            int maxInput =-1 ;
            int kInput = -1;
            float kFloatInp; 
            cout << "Query cost, vary k experiment" << endl;
            
            //input for query range and k
            cout << "Please enter the min hilbert values to search: " ;
            while (!(cin >> minInput)) {
                cout << "Invalid input. Please enter an integer: ";
            }
            cout << "Please enter the max hilbert values to search: " ;
            while (!(cin >> minInput)) {
                cout << "Invalid input. Please enter an integer: ";
                cin >> minInput;
            }
            cout << "Now please enter k (0.02, 0.04, 0.06, 0.08, 0.1): " ;
            while (!(cin >> kFloatInp)) {
                cout << "Invalid input. Please enter one (0.02, 0.04, 0.06, 0.08, 0.1): ";
                cin >> kFloatInp; 
            }
            kInput = totalNumRecords * kFloatInp; 
            //cout << kInput << endl; 
            cout << "Starting experiment... " << endl; 


            auto startQueryK = chrono::high_resolution_clock::now();
            vector<Record> results = tree.querying(minInput, maxInput, kInput); 
            //cout << "getting out of querying" << endl; 

            //printed out for smaller values and testing
            /*for (size_t i = 0; i < results.size(); i++)
            {
                cout << "results ID: " << results[i].id
                << ", Lat: " << results[i].lat
                << ", Lon: " << results[i].lon
                << ", Time: " << results[i].timestamp
                << ", Hilbert: " << results[i].hilbert << endl; 
                
            }*/
            
            //ends timer after sorted data set is complete, calculates elapsed time
            auto endQueryK = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_time = endQueryK - startQueryK;
            cout << "RangeQuery Cost: " << total_time.count() << " seconds" << endl;

        } else if (experimentInput == 2) {
            //query cost vary q
            int minInput = -1;
            int maxInput =-1 ;
            int kInput = -1;
            //kInput = 5000; //5000 or 10000
            cout << "Query cost, vary q experiment" << endl;
            cout << "In this experiment, k is set to 5000 and 10000 respectively" << endl;
            
            cout << "Please enter the min hilbert values to search: " ;
            //https://www.geeksforgeeks.org/how-to-validate-user-input-in-cpp/
            while (!(cin >> minInput)) {
                cout << "Invalid input. Please enter an integer: ";
            }
            cout << "Please enter the max hilbert values to search: " ;
            while (!(cin >> minInput)) {
                cout << "Invalid input. Please enter an integer: ";
            }
            //inputed so easier when running experiments
            cout << "Please input k as either 5000 or 10000: " ; 
            while (!(cin >> kInput)) {
                if(kInput != 5000 || kInput != 10000){
                    break;
                }
                cout << "Invalid input. Please enter one (0.02, 0.04, 0.06, 0.08, 0.1): ";
            }
            cout << "Starting experiment... " << endl; 

            auto startQueryQ = chrono::high_resolution_clock::now();
            vector<Record> results = tree.querying(minInput, maxInput, kInput); 

            //printed out for smaller values and testing
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


    file.close();


    //cleans up disk directory
    int status1 = system("rm -rf ls_tree_pages");
    if (status1 == 0)
        cout << "removed ls_tree_pages" << endl; 
    else
        cerr << "error in cleanup" << endl;

      //cleans up memory directory
      int status2 = system("rm -rf /tmp/inMemoryTree");
      if (status2 == 0)
          cout << "removed /tmp/inMemoryTree" << endl; 
      else
          cerr << "error in cleanup" << endl;
  

    return 0;
}