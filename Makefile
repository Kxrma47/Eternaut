CXX := c++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2
TARGET := inter_list
SRC := main.cpp

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET)
