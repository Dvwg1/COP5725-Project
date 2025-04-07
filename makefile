#Makefile used to generate the base model r-tree, rs tree, ls tree, and disk sort

CXX = g++  
CXXFLAGS = -std=c++17 -O2 -Wall  

TARGET = h_rtree  
SRCS = base_model_rtree.cpp rtree.cpp  

SORT_TARGET = sort  
SORT_SRC = disk_based_sort.cpp  

all: $(TARGET) $(SORT_TARGET)

$(TARGET): $(SRCS)  
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

$(SORT_TARGET): $(SORT_SRC)  
	$(CXX) $(CXXFLAGS) -o $@ $(SORT_SRC)

clean:  
	rm -f $(TARGET) $(SORT_TARGET)  
	rm -rf tree_pages/ *.dot *.png
