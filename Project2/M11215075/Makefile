CXX = g++
CXXFLAGS = -g -std=c++17
TARGET = cts

all: $(TARGET)

$(TARGET): M11215075.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
