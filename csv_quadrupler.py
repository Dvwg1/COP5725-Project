"""
used to 4x the OSM data set, custom written
"""

from tqdm import tqdm

original_file = "osm_na_node_data.csv"
new_file = "osm_na_node_data_4x.csv"


#open the original csv
with open(original_file, 'r', encoding='utf-8', errors='ignore') as src:
    header = src.readline()

with open(new_file, 'w', encoding='utf-8') as dst:

    #write the header once
    dst.write(header)

    #write 4 times, ie quadrupling
    for i in range(4):
        print(f"Writing pass {i+1}/4...")
        with open(original_file, 'r', encoding='utf-8', errors='ignore') as src:
            next(src)  # skip header again
            for line in tqdm(src, unit='lines', desc=f"Pass {i+1}"):
                dst.write(line)

