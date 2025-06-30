CXX = g++
CXXFLAGS = -std=c++17 -Wall
LIBS = -luser32 -lgdi32
TARGET = YouTubeLinkCopier.exe
SOURCE = main.cpp

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) $(LIBS)

clean:
	del $(TARGET)

.PHONY: clean 