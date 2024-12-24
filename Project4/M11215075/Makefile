CC = g++
CFLAGS = -Wall -O2 -std=c++17

UNAME_S := $(shell uname -s)

# Base Gurobi configuration
ifeq ($(UNAME_S), Darwin)
    GUROBI_DIR = /Library/gurobi1200/macos_universal2
    LIB_EXT = dylib
else
    GUROBI_DIR = /opt/gurobi1200/linux64
    LIB_EXT = so
endif

GUROBI_LIBS = -L$(GUROBI_DIR)/lib -lgurobi_c++ -lgurobi120
GUROBI_INC = -I$(GUROBI_DIR)/include

# Validation
CHECK_GUROBI_INC := $(wildcard $(GUROBI_DIR)/include/gurobi_c++.h)
CHECK_GUROBI_LIB := $(wildcard $(GUROBI_DIR)/lib/libgurobi120.$(LIB_EXT))

ifeq ($(CHECK_GUROBI_INC),)
    $(error Cannot find gurobi_c++.h in $(GUROBI_DIR)/include)
endif

ifeq ($(CHECK_GUROBI_LIB),)
    $(error Cannot find libgurobi120.$(LIB_EXT) in $(GUROBI_DIR)/lib)
endif

SRCS = M11215075.cpp parser.cpp list_scheduling.cpp ilp.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = mlrcs

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(GUROBI_LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) $(GUROBI_INC) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

debug:
	@echo "OS: $(UNAME_S)"
	@echo "Gurobi: $(GUROBI_DIR)"
	@echo "Lib ext: $(LIB_EXT)"