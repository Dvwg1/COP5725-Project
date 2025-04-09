#Makefile used to generate the base model r-tree, rs tree, ls tree, and disk sort

CXX = g++  
CXXFLAGS = -std=c++17 -O2 -Wall  

TARGET = h_rtree  
SRCS = base_model_rtree.cpp rtree.cpp  

SORT_TARGET = sort  
SORT_SRC = disk_based_sort.cpp 

LS_TARGET = lstree
LS_SRCS = base_model_lstree.cpp LSTree.cpp rtree.cpp

all: $(TARGET) $(SORT_TARGET) $(LS_TARGET)

$(TARGET): $(SRCS)  
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

$(SORT_TARGET): $(SORT_SRC)  
	$(CXX) $(CXXFLAGS) -o $@ $(SORT_SRC)

$(LS_TARGET): $(LS_SRCS)  
	$(CXX) $(CXXFLAGS) -o $@ $(LS_SRCS)

clean:  
	rm -f $(TARGET) $(SORT_TARGET)  
	rm -rf tree_pages/ *.dot *.png
	rm -rf ls_tree_pages/ lstree inMemoryTree/
