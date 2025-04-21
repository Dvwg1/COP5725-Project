# COP5725-Project
Semester Project based on implementation on "Spatial Online Sampling and Aggregation"

Directory Tree

├── demo_and_test_csv
│   ├── andorra_test.csv
│   ├── andorra_test_sorted.csv
│   ├── values1123.csv
│   ├── values226.csv
│   └── values226_no_hilbert.csv
├── documentation
│   └── Guide on Downloading OSM Data.pdf
├── helper_scripts
│   ├── csv_doubler.py
│   ├── csv_halfer.py
│   ├── csv_oneandhalfer.py
│   ├── csv_quadrupler.py
│   ├── geolife_to_csv.py
│   ├── text_to_csv2.py
│   └── utility.py
├── project_code
│   ├── base_model_lstree.cpp
│   ├── base_model_rtree.cpp
│   ├── BPlusTree.cpp
│   ├── disk_based_sort.cpp
│   ├── hilbert.h
│   ├── LSTree.cpp
│   ├── LSTree.hpp
│   ├── makefile
│   ├── RStree.cpp
│   ├── RStree.hpp
│   ├── RS-tree_main.cpp
│   ├── rtree.cpp
│   └── rtree.hpp
└── README.md

**DATA PROCESSING**
In order to get the data sets, documentation/Guide on Downloading OSM Data.pdf was used to get the OSM DC data set in a .osm.pbf file. 
The Geolife data set is from Microsoft Research, https://download.microsoft.com/download/F/4/8/F4894AA5-FDBC-481E-9285-D5F8C4C4F039/Geolife%20Trajectories%201.3.zip.
The helper script, geolife_to_csv.py, was used to convert the data set to a .csv file for experimentation. 

The osm.pbf file had to be converted to .csv, which was done by first converting it to a .txt file with the Osmium library. 
The Osmium command used was 'osmium cat district-of-columbia-latest.osm.pbf -f osm | grep "<node" > nodes.txt' which wrote every node in the .osm.pbf file into the .txt file.
Once a .txt file included all of the nodes from the .osm.pbf file, they were converted to .csv with helper_scripts/test_to_csv2.py
This would result in a .csv files like andorra_test.csv, but it was still not ready for experimentation yet. 

The .csv files, both for OSM and Geolife, still needed to be sorted by their hilbert values first before being used for experiments.
This was done using the disk_based_sort.cpp, which generates a hilbert values using hilbert.h and sorts the whole csv from the lowest hilbert value to the highest. 
An example of this would be using andorra_text.csv as the input file for disk_based_sort.cpp and it would result in andorra_test_sorted.csv. 
The values226_no_hilbert.csv was also used to test this and created the values226.csv. 

**HELPER SCRIPTS**
The rest of the files in the helper_scripts directory, excpet for utility.py, are used for getting the data sets for the scaling experiment on the RS-Tree. 
The csv_doubler.py is to double the size of the OSM DC data set. The csv_halfer.py cuts the size of the OSM DC data set in half and the csv_quadrupler.py quadruples the OSM DC data set.
The last file in the helper_scripts directory, utility.py, was used in testing to vizualize the nodes and records inside of the R-Tree and LS-Tree. 

**PROJECT CODE**
makefile - compiles four different executables: sort - sorting algorithm, h_rtree - R-Tree, lstree - LS-Tree, rs_tree - RS-Tree
The sorting algorithm is explained in the Data Processing section. 
The compilation of h_rtree uses base_model_rtree.cpp and rtree.cpp to build an disk-based R-Tree sorted by hilbert values. A menu is given to run experiments on the R-Tree once it is built in base_model_rtree.cpp. 
The rtree.cpp is the source file to the header file, rtree.hpp. 

The compilation of lstree uses base_model_lstree.cpp, LSTree.cpp, and rtree.cpp to build the LS-Tree, with the smallest tree being stored in memory. A menu is given to run experiments on the LS-Tree once it is built in base_model_lstree.cpp.
The LSTree.cpp is the source file to the header file, LSTree.hpp. 

The compilation of rs_tree uses RS-tree_main.cpp, RStree.cpp. A menu is given to run experiments on the RS-Tree once it is built in RS-tree_main.cpp.
The RStree.cpp is the source file to the header file, Rstree.hpp. 

**HOW TO RUN FILES**
Once the makefile is complete and creates the executables, each one can run with ./executable. 
MAKE SURE TO INPUT THE WHOLE PATH FOR FILE INPUT. This can be found using the pwd command. 
PLEASE NOTE: Due to time constraints, some of the input options do not support all input types. Unexpected input may cause endless loops or the program to crash. 
The sorting algorithm, sort, asks for an input csv file that wil be sorted and the output file to send the resulted sorted hilbert values to. 
The R-Tree, h_rtree, asks the user to input the sorted csv file and builds the disk-based R-Tree from it. Once the tree is built, a menu appears to run other experiments. When entering 1, the update experiment begins and will end the program once done. 
The LS-Tree, lstree, asks the user to input the sorted csv file and builds the LS-Tree from it. Once the tree is built, a menu appears to run other experiments. 
When entering 1, the vary k experiment prompts the user for the query results, the minimum and maximum hilbert values and the value of k as a float. 
When entering 2, the vary q experiment prompts the user for the query results, the minimum and maximum hilbert values and the value of k as either 5000 or 10000.
When entering 4, the update experiment begins and will end the program once done. 

The RS-Tree, rs_tree, asks the user to input the sorted csv file and builds the RS-Tree from it. Once the tree is built, a menu appears to run other experiments. When entering 1, the update experiment begins and will end the program once done.
