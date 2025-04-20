"""
joins two CSV files linearly, custom written
"""

from tqdm import tqdm

file_a = "osm_dc.csv"
file_b = "osm_dc_half.csv"
output_file = "osm_dc_onenhalf.csv"

with open(file_a, 'r', encoding='utf-8', errors='ignore') as fa, \
     open(file_b, 'r', encoding='utf-8', errors='ignore') as fb:

    header_a = fa.readline()
    header_b = fb.readline()

    if header_a.strip() != header_b.strip():
        raise ValueError("Headers do not match between the two files.")

    lines_a = fa.readlines()
    lines_b = fb.readlines()

with open(output_file, 'w', encoding='utf-8') as out:
    out.write(header_a)

    for line in tqdm(lines_a + lines_b, desc="Merging files"):
        out.write(line)

