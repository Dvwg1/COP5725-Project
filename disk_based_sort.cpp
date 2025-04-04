/*

// --Disk Based Sorting Algorithm--

References: https://www.geeksforgeeks.org/external-sorting/

Disk Based Sorting Algorithm to be used for getting the .csv data sorted before R-tree, RS-tree, and LS-tree are built. 
Because Wang et al. gives no information regarding the disk based sorting algorithm's implementation, we used an 
external merge sort which splits the data into chunks that fit within memory, sorts the chunks in memory by calculated Hilbert 
value, saves the chunks to disk, and then merges the sorted chunks into a final sorted .csv file. The resulting output csv 
file is sorted by Hilbert value as specified in Wang et al. Inspiration for the merging algorithm used is from geeksforgeeks.

Run this on the desired .csv to be used in tree construction. Will perform an external merge sort by Hilbert value

*/

//hilbert
#include "hilbert.h"

//used for input and data manipulation
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>

//dyanmic data structures and data types used
#include <vector>
#include <queue>
#include <cstdint>
#include <string>

//needed for calculations
#include <algorithm>
#include <utility>
#include <chrono>

//used to get system info needed for chunk size calculations
#include <sys/sysinfo.h>

using namespace std;

//global min and max coord values used for hilbert calc
double lat_min, lat_max, lon_min, lon_max;

//custom structure for records from both geolife and osm .csv files
struct record{

	//each represent the elements in a record 
	string id;
	string latitude;
	string longitude;
	string timestamp;

	//hilbert_value to be used for sorting
	int hilbert_value;

	//sorts the records by hilbert value, ascending
	bool operator<(const record& other) const {
		return hilbert_value < other.hilbert_value; 

	}

};

//heap node to be used for k sorting, as used in geeksforgeeks external merge sorting
struct heap_node{

	//record instance
    record record_instance;

    //source file index
    int file_index; // Index of source file

    //sorts records by hilbert value, ascending
    bool operator>(const heap_node& other) const {
        return record_instance.hilbert_value > other.record_instance.hilbert_value;
    }
};


//used to dynamically calculate the best chunk size to use, based on current system specs and assuming 
//only up to 10% of available RAM is to be used, as to maintain overall system performance
size_t chunk_calculator();

//used to parse a record as a line into the record structure, also calculates hilbert value for a record
record parse_line_and_calc_hilbert(const string& line, double lat_min, double lat_max,
                                double lon_min, double lon_max);

//used to read in chunk of lines from input.csv, returns chunk_size amount of records in vector form
vector<record> records_to_chunks(ifstream& filename, size_t chunk_size);

//used to write a record chunk into a temporary .csv file
void chunks_to_temp_csv(const vector<record>& record_vector, int chunk_id);

//used to merge chunks into a sorted .csv file, using priority queue
void merge_chunks(int num_chunks, const string& filename);

//main function to take in user input
int main(){

	//files to be used
	string input_csv;
	string output_csv;

	//allows user to enter the input file and desired output file name
	cout<<"Enter input file (must end in .csv): ";
	getline(cin, input_csv);

	cout<<"Enter desired output file (please end with .csv): ";
	getline(cin, output_csv);
	
	//loads input_csv into in for reading
	ifstream in(input_csv);

	//error checking for input file
	if (!in.is_open()){

		cerr<<"ERROR: .csv file not found/can't be opened!" << endl;
		return 1;
	}

	//starts the timer, at this point the file should have been found
	auto start = chrono::high_resolution_clock::now();

	//saves the header from the input file to be used later'
	string header;
	getline(in, header);

	//debugging
	//cout<<chunk_calculator()<<endl;head 

	//size of a chunk based on chunk_calculator()
	size_t chunk_size = chunk_calculator();

	//use to find the max and min longitude and latitude values needed for hilbert value calculations
	max_and_min_finder(input_csv);

	//create instance of record vector
	vector<record> record_vector;

	//beginning chunk_id
	int chunk_id = 0;

	//loops until no more records
	while (true) {

		//populates record_vector with records
		record_vector = records_to_chunks(in, chunk_size);

		//checks if there are no more values in records
		if (record_vector.empty() == true){
			break;
		}

		//run sort to sort the records in the record_vector by hilbert value, uses bool defined operator
		sort(record_vector.begin(), record_vector.end());

		//create chunk file
		chunks_to_temp_csv(record_vector, chunk_id++);

	}


	//closes the input.csv
	in.close();

	//output.csv instance
	ofstream out(output_csv);

	//writes the header to the output_file
	out << "id,latitude,longitude,timestamp,hilbert_value" << "\n";
	out.close();

	//ends timer after sorted data set is complete, calculates elapsed time
	auto end = std::chrono::high_resolution_clock::now();
	chrono::duration<double> total_time = end - start;
	cout << "Total sorting time elapsed: " << total_time.count() << " seconds" << endl;

	//merge all chunk files into one gigafile
	merge_chunks(chunk_id, output_csv);

	//removes the residual chunk_0.csv file
	filesystem::remove("chunk_0.csv");

	return 0;

}


/*function definitions*/

//dynamic chunk size calculator
size_t chunk_calculator(){

	//init instance of system info variable
	struct sysinfo info;
	sysinfo(&info);

	//calculate available memory to use by multiplying the avaible memory size by the memory unit
	size_t available_memory = info.freeram * info.mem_unit;
	//from this, only going to use 25% to avoid issues
	size_t actual_memory = available_memory / 25;

	//using a sample OSM csv file, divided its file size (31732283 bytes) by total number of records (457809) to
	//determine that each record is about 69 bytes. I rounded to 80 for some padding just in case scaling increases
	//record size (though it shouldn't)
	size_t record_size = 80;

	//return the actual free memory that can be used / the record size - corresponds to chunk size
	return (actual_memory/record_size)	;

}

record parse_line_and_calc_hilbert(const string& line, double lat_min, double lat_max,
                                double lon_min, double lon_max){

	//read in the line
	stringstream ss(line);

	//record elements as variables
	string id, lat, lon, ts;

	//extract the values from the line
	getline(ss, id, ',');
    getline(ss, lat, ',');
    getline(ss, lon, ',');
    getline(ss, ts, ',');


    //convert lon and lat to doubles for hilbert value calculation
    double latitude = stod(lat);
    double longitude = stod(lon);

    //calculate hilbert using the hilbert.h function, wiht p set to 8
    int hilbert = coords_to_hilbert_value(latitude, longitude, lat_min, lat_max, lon_min, lon_max, 8);

    //return record instance
    return {id, lat, lon, ts, hilbert};
}


vector<record> records_to_chunks(ifstream& filename, size_t chunk_size){

	//create a vector of record class
	vector<record> record_vector;

	//used to store line from input.csv
    string line;

    //initial counter value
    size_t count = 0;

    //loops through input.csv until reaches max chunk size limit or end of .csv
    while (count < chunk_size && getline(filename, line)) {

    	//parses the line, gets the hilbert value, and adds to record_vector
        record_vector.push_back(parse_line_and_calc_hilbert(line, lat_min, lat_max, lon_min, lon_max));

        //increment counter
        count++;
    }

    //return vector 
    return record_vector;
}


void chunks_to_temp_csv(const vector<record>& record_vector, int chunk_id){

	//creates a file using the provided chunk_id and an outstream instance
	ofstream out("chunk_" + to_string(chunk_id) + ".csv");

	//adds record values into .csv file
    for (size_t i = 0; i < record_vector.size(); i++) {
        out << record_vector[i].id << "," << record_vector[i].latitude << "," << record_vector[i].longitude << "," << record_vector[i].timestamp << "," << record_vector[i].hilbert_value << "\n";
    }
}


void merge_chunks(int num_chunks, const string& filename){

	//creates vector of input streams for each chunk file
	vector<ifstream> inputs(num_chunks);

	//creates priority queue based on the custom heap_node struct, defined as min_heap 
    priority_queue<heap_node, vector<heap_node>, greater<>> min_heap;

    //go through each of the chunks
    for (int i = 0; i < num_chunks; ++i) {

    	//opens the file based on the iteration
        inputs[i].open("chunk_" + to_string(i) + ".csv");

        //using line, pushes first records into min_heap
        string line;
        if (getline(inputs[i], line)) {
            min_heap.push({parse_line_and_calc_hilbert(line, lat_min, lat_max, lon_min, lon_max), i});
        }
    }

    //create output stream instance to write to output file
    ofstream out(filename, ios::app);

    //does this process until no more records to process
    while (!min_heap.empty()) {

    	//using unstructured binding to intilialize rec to record and index to file_index from the top of min_heap
        auto [rec, idx] = min_heap.top();

        //pops value from min_heap
        min_heap.pop();

        //sanitize the timestamp to ensure no line breaks, removes troublesome spacing
		string ts_clean = rec.timestamp;
		ts_clean.erase(remove(ts_clean.begin(), ts_clean.end(), '\n'), ts_clean.end());
		ts_clean.erase(remove(ts_clean.begin(), ts_clean.end(), '\r'), ts_clean.end());

		//write sanitized record to output
		out << rec.id << "," << rec.latitude << "," << rec.longitude << "," << ts_clean << "," << to_string(rec.hilbert_value) << "\n";


        //read next line from the same file, if next line, push to heap
        string next_line;
        if (getline(inputs[idx], next_line)) {
            min_heap.push({parse_line_and_calc_hilbert(next_line, lat_min, lat_max, lon_min, lon_max), idx});
        }
    }

    //close all input files
    for (size_t i = 0; i < inputs.size(); ++i) {
    	inputs[i].close();
	}

}
