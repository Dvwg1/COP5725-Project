"""

Custom script written to convert osm.txt data into .csv data with the required latitude, longitude, 12 byte unique ID, and
timestamp. The script generates the IDs and converts the timestamp to match the geolife timestamps. Meant to work with 
high load data

"""



import re
import uuid
import csv
from datetime import datetime
from tqdm import tqdm

#file paths
input_file = 'north_america_nodes.txt'    
output_file = 'osm_na_node_data.csv'

#regex pattern to extract timestamp, lat, lon
pattern = re.compile(r'timestamp="([^"]+)"\s+lat="([^"]+)"\s+lon="([^"]+)"')

#opens the input file
with open(input_file, 'r') as infile, open(output_file, 'w', newline='') as outfile:

    #write as csv, with the needed fields
    writer = csv.writer(outfile)
    writer.writerow(['id', 'latitude', 'longitude', 'timestamp'])

    #searches for patterns, line by line while using tqdm to give an alotted time as well as iteration counter
    for line in tqdm(infile, desc="parsing lines"):
        match = pattern.search(line)

        #adds data if found
        if match:
            timestamp_raw, lat, lon = match.groups()
            
            #convert timestamp to same format used in geolife
            timestamp = datetime.strptime(timestamp_raw, '%Y-%m-%dT%H:%M:%SZ').strftime('%Y-%m-%d %H:%M:%S')
            
            #generate unique 12-byte ID for each record
            uid = uuid.uuid4().hex[:24]
            
            #write directly to the CSV
            writer.writerow([uid, lat, lon, timestamp])
