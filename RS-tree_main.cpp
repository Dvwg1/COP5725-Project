//--- RS-Tree ---

/*
This is used to construct an RS-tree using the h_rtree header files.
Used in some experiments as is
*/

// header files
#include "RStree.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <string>
#include <queue>
#include <chrono>
#include <cstdlib>
#include <random>  
#include <chrono>
#include <cstdlib>

//for count()
#include <bits/stdc++.h>

using namespace std;

int main() {

    //random seeding
    srand(time(0));
    unsigned seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    string inputFile;
    cout << "Enter CSV file path: ";
    getline(cin, inputFile);

    ifstream file(inputFile);
    if (!file.is_open()) {
        cerr << "Failed to open file." << endl;
        return 1;
    }

    //starts the timer, at this point the file should have been found
	auto start = chrono::high_resolution_clock::now();

    //initialize memory-based RS-tree
    b_plus_tree tree("RStree_pages");

    //used to store the record counter
    long long int num_records = 0;

    string line;
    getline(file, line);

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

                tree.insert(r.hilbert, r, true);

                //incremente record counter
                num_records++;
            } 
            
            //error catching for debugging
            catch (const exception& e) {
                cerr << "Invalid line (skipping): " << line << "\\n";
                cerr << "  Error: " << e.what() << "\\n";
            }
        } 
    }


    file.close();

    //do sampling on tree before it can be said to have finished construction
    tree.buildAllSamples();

    //ends timer after sorted data set is complete, calculates elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_time = end - start;
    cout << "RS-Tree built from CSV and stored on disk.\n";
	cout << "Total sorting time elapsed: " << total_time.count() << " seconds" << endl;
    cout << "Internal nodes loaded into memory, volatile" << endl;


    //menu to select what experiment to do next
    int experimentInput = -1;

    do {
        cout << endl << endl << "Experiments to run: " << endl;

        cout << "1. Normal Update cost (FYI this experiment will end program and tree will have to be built again): " << endl;
        cout << "2. Vary q Update cost (FYI this experiment will end program and tree will have to be built again): " << endl;
        cout << "3. Exit program" << endl; 
        cout << "Please enter the experiment to run: ";
        cin >> experimentInput; 
        if (experimentInput == 3) 
            break; 

        else if (experimentInput == 1) {

            //NORMAL UPDATE COST EXPERIMENT (INSERTIONS AND DELETIONS)
            cout << "Update Cost experiment now beginning" << endl ;
            cout << "Now inserting 5000 records into RS-Tree" << endl; 

            //the actual content of the records should not matter, and so
            //the first 25,000 records are recovered, shuffled, and then
            //from those, the first 5,000 actually get inserted.
            
            //this retrieval is not timed, as it is just getting the values
            cout << "Acquiring values from the dataset" << endl;

            ifstream file(inputFile);
            if (!file.is_open()) {
                cerr << "Failed to open file." << endl;
                return 1;
            }

            string line;
            getline(file, line);

            //self explanatory
            vector<Record> records_to_be_shuffled;

            //num records 2 electric boogaloo
            int num_records2 = 0;

            Record test_r;

            //modified from how a tree is built, except its only gonna read in the first 25000
            //records and shove them into a vector (hopefully big enough)
            while (getline(file, line) && num_records2 < 1000000) {

                stringstream ss(line);
                string idStr, latStr, lonStr, tsStr, hStr;
        
                if (getline(ss, idStr, ',') &&
                    getline(ss, latStr, ',') &&
                    getline(ss, lonStr, ',') &&
                    getline(ss, tsStr, ',') &&
                    getline(ss, hStr, ',')) {
        
                    try {
                       
                        strncpy(test_r.id, idStr.c_str(), sizeof(test_r.id));
                        test_r.id[sizeof(test_r.id) - 1] = '\0';
        
                        test_r.lon = stof(lonStr);
                        test_r.lat = stof(latStr);
                        strncpy(test_r.timestamp, idStr.c_str(), sizeof(test_r.timestamp));
                        test_r.timestamp[sizeof(test_r.timestamp) - 1] = '\0';
                        test_r.hilbert = stoi(hStr);
        
                        //pushes back the record

                        cout << "record being inserted" << test_r.hilbert << endl;
                        records_to_be_shuffled.push_back(test_r);
        
                        //incremente record counter
                        num_records2++;
                    } 
                    
                    //error catching for debugging
                    catch (const exception& e) {
                        cerr << "Invalid line (skipping): " << line << "\\n";
                        cerr << "  Error: " << e.what() << "\\n";
                    }
                } 
            }

            cout << "num_records2" << num_records2 << endl;
        
        
            file.close();

            //shuffle
            shuffle(records_to_be_shuffled.begin(), records_to_be_shuffled.end(), default_random_engine(seed));
            shuffle(records_to_be_shuffled.begin(), records_to_be_shuffled.end(), default_random_engine(seed));
            shuffle(records_to_be_shuffled.begin(), records_to_be_shuffled.end(), default_random_engine(seed));
            shuffle(records_to_be_shuffled.begin(), records_to_be_shuffled.end(), default_random_engine(seed));
            shuffle(records_to_be_shuffled.begin(), records_to_be_shuffled.end(), default_random_engine(seed));



            //after data found and shuffled, insertion counter starts tbh
            auto startInsert = chrono::high_resolution_clock::now();

            //capture time for every 50 insertions
            vector<double> insertTimes; 
            int increments = 50; 
            for (int i = 0; i < 5001; i++)
            {
                   //incremental time keeping
                    if (i == increments) {

                        //incremental time calculations
                        auto endIncrement = std::chrono::high_resolution_clock::now();
                        chrono::duration<double> total_timeIncrement = endIncrement - startInsert;
                        insertTimes.push_back(total_timeIncrement.count()); 


                        if (increments % 1000 == 0) //print out when 1000s
                            cout << "increment at " << i << " is " << total_timeIncrement.count() << endl ;
                        increments += 50; 
                    }

                    //rs tree insertion
                    tree.insert(test_r.hilbert, test_r, false);

                    num_records++;
                    cout << "record just inserted: " << test_r.hilbert << endl; 
            }

            //cout << "num records after insertion: " << totalNumRecords << endl; 

            //ends timer after sorted data set is complete, calculates elapsed time
            auto endInsert = std::chrono::high_resolution_clock::now();
            chrono::duration<double> total_timeInsert = endInsert - startInsert;
            cout << "Insertion Cost: " << total_timeInsert.count() << " seconds" << endl;

            cout << "num records after insertion: " << num_records << endl; 

            //give yourself a lil break between inserts and deletes 
            sleep(2); 

            //now after inserting those records, they need to be removed. 
            cout << endl << "Now Cost of Deletion will be recorded. " << endl; 


            chrono::duration<double> total_timeIncrement;
            vector<double> deletionsTimes; 
            //reset increments for deletions
            increments = 50; 
            auto startDeletion = std::chrono::high_resolution_clock::now();

            for (int i = 0; i < 5001; i++)
            {
               
                if (i == increments) {
                    auto endIncrement = std::chrono::high_resolution_clock::now();
                    total_timeIncrement = endIncrement - startInsert;
                    deletionsTimes.push_back(total_timeIncrement.count()); 
                    if (increments % 1000 == 0) //print out when 1000s
                        cout << "increment at " << i << " is " << total_timeIncrement.count() << endl ;
                    increments += 50; 
                }
                tree.remove(records_to_be_shuffled[i].hilbert); 
                num_records--;
            }
            cout << "num records after deletion: " << num_records << endl; 


            auto endDeletion = std::chrono::high_resolution_clock::now();
            cout << "Deletion Cost: " << total_timeIncrement.count() << " seconds" << endl;
            

        }
     

    } while(experimentInput != 1 || experimentInput != 2) ;


    

    //cleans up disk directory
    int status = system("rm -rf RStree_pages");
    if (status == 0)
        cout << "removed RStree_pages" << endl; 
    else
        cerr << "error in cleanup" << endl;
    

    return 0;
}
