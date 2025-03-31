
"""
Reference:  https://gist.github.com/AlexandraKapp/296f79e77e3b7772f8af486a72689da0


This script is a modified version of Alexandra Kapp's geolife_to_csv.py script, with our own key modifications which include: removing the geo boundaries filtration,
taking only the required elements (latitude, longitude, time), keeping the times unmodified, and adding a 12 byte ID to each record

While this is not an implementation of a method/design from Wang et al., it is needed to convert and parse the geolife data used in the implemented experiments

"""



import os
import csv
import uuid
import pandas as pd
import numpy as np
from io import BytesIO
from zipfile import ZipFile
from urllib.request import urlopen
from tqdm.auto import tqdm
from pathlib import Path

##### INPUT VARIABLES #####
# set path names
RAW_DATA_PATH = "raw/geolife"
PROCESSED_DATA_PATH = "preprocessed"


############ Download data ###############
# GEOLIFE
# download GeoLife data if necessary
if not os.path.exists(RAW_DATA_PATH):
    with tqdm(total=1, desc="Download geolife data") as pbar:
        os.makedirs(RAW_DATA_PATH)
        url = "https://download.microsoft.com/download/F/4/8/F4894AA5-FDBC-481E-9285-D5F8C4C4F039/Geolife%20Trajectories%201.3.zip"
        with urlopen(url) as zipresp:
            with ZipFile(BytesIO(zipresp.read())) as zfile:
                zfile.extractall(RAW_DATA_PATH)
        pbar.update()
else:
    print("Geolife data already exists. Download skipped.")

############ Preprocess data ###############

#### FUNCTIONS ####
# clean header of plt files and write all data into single csv
def preprocess_plt(file_path):
    # read plt file
    with open(file_path, "r") as fin:
        rows = list(csv.reader(fin))[6:]  # Skip header
        return [[float(row[0]), float(row[1]), row[5] + " " + row[6]] for row in rows]

#MODIFIED: collect and preprocess data
#will go through each .plt file, join, extend, and return needed records and their element
def collect_data(base_dir):
    records = []
    users = sorted(os.listdir(base_dir))
    with tqdm(users, desc="Processing GeoLife data") as pbar:
        for user_id in pbar:
            traj_dir = os.path.join(base_dir, user_id, "Trajectory")
            for traj_file in os.listdir(traj_dir):
                if traj_file.endswith(".plt"):
                    file_path = os.path.join(traj_dir, traj_file)
                    records.extend(preprocess_plt(file_path))
    return pd.DataFrame(records, columns=["latitude", "longitude", "timestamp"])

# Main script
geolife_dir = os.path.join(RAW_DATA_PATH, "Geolife Trajectories 1.3", "Data")
df = collect_data(geolife_dir)

# generate unique 12-byte ID for each record
df["id"] = [uuid.uuid4().hex[:24] for _ in range(len(df))]

# convert timestamp to datetime (without timezone conversion)
df["timestamp"] = pd.to_datetime(df["timestamp"])

# save to CSV
if not os.path.exists(PROCESSED_DATA_PATH):
    os.makedirs(PROCESSED_DATA_PATH)


# CSV output with necessary fields
output_csv_path = os.path.join(PROCESSED_DATA_PATH, "geolife_unique_id.csv")
df.to_csv(output_csv_path, index=False, columns=["id", "latitude", "longitude", "timestamp"])

print(f"CSV file created: {output_csv_path}")
