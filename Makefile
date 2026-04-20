CXX      = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra -Wno-unused-result
TARGET   = dungeon

all: $(TARGET)

$(TARGET): game.cpp
	$(CXX) $(CXXFLAGS) -o $(TARGET) game.cpp

clean:
	rm -f $(TARGET)

.PHONY: all clean
