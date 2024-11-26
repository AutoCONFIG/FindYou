# Compiler and flags
CXX = g++
CXXFLAGS = -I./include -std=c++17 -Wall -Wextra -O3
LDFLAGS = -lmaxminddb

# Project structure
SRCDIR = src
INCDIR = src/include
TARGET = findyou
SRC = $(SRCDIR)/findyou.cpp
OBJ = $(SRC:.cpp=.o)

# Default target
all: $(TARGET)

# Compile the target
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Compile source files
$(SRCDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets
.PHONY: all clean