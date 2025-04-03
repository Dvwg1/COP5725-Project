#Makefile used to generate the base model r-tree, rs tree, and ls tree source code

CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall
TARGET = h_rtree
SRCS = based_model_rtree.cpp rtree.cpp

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $@ $(SRCS)

clean:
	rm -f $(TARGET)
	rm -rf tree_pages/ *.dot *.png
