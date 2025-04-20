"""
used to 0.5x the OSM data set, custom written
"""

from tqdm import tqdm

original_file = "osm_dc.csv"
new_file = "osm_dc_half.csv"

#get the total number of lines
with open(original_file, 'r', encoding='utf-8', errors='ignore') as src:
    total_lines = sum(1 for _ in src) - 1  

#get half the lines
half_lines = total_lines // 2

with open(original_file, 'r', encoding='utf-8', errors='ignore') as src:
    header = src.readline()

    with open(new_file, 'w', encoding='utf-8') as dst:
        dst.write(header)
        for i, line in enumerate(tqdm(src, total=half_lines, desc="Writing 50%")):
            if i >= half_lines:
                break
            dst.write(line)