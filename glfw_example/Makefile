CXX = g++
CXXFLAGS = -g -std=c++17 -O2
LDFLAGS = -lglfw -lvulkan

TARGET = Cthovk_Example
SOURCES = $(wildcard ../src/*.cpp) main.cpp
HEADERS = $(wildcard ../headers/*.h)

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $(SOURCES) $(LDFLAGS)

test: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
