#Project makefile

CXX = g++
CXXFLAGS = -std=c++20 -Wall
SORT = disk_based_sort
RTREE = h_rtree
SORT_SRC = disk_based_sort.cpp 
RTREE_SRC = BPlusTree.cpp

all: $(SORT) $(RTREE)

$(SORT): $(SORT_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(RTREE): $(RTREE_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

#removes the executable and the chunk csv files
clean:
	rm -f $(SORT) $(RTREE) chunk_*.csv

