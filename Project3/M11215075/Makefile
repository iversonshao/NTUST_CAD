# Compiler settings
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O3

# File names
TARGET = legalizer
SOURCE = M11215075.cpp

# Output directories
OUTPUT_DIRS = output1 output2 output3

# Build rules
all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

# Clean build files and test outputs
clean:
	rm -f $(TARGET)
	rm -rf $(OUTPUT_DIRS)
	rm -f *_input_placement.png *_output_placement.png
	rm -f *.gp

# Clean only test outputs but keep executable
clean_output:
	rm -rf $(OUTPUT_DIRS)
	rm -f *_input_placement.png *_output_placement.png
	rm -f *.gp

# Run example
test: test1 test2 test3

test1: $(TARGET)
	@echo "Testing case1..."
	@mkdir -p output1
	./$(TARGET) bench/toy output1

test2: $(TARGET)
	@echo "Testing case2..."
	@mkdir -p output2
	./$(TARGET) bench/ibm01 output2

test3: $(TARGET)
	@echo "Testing case3..."
	@mkdir -p output3
	./$(TARGET) bench/ibm05 output3

.PHONY: all clean clean_output test test1 test2 test3