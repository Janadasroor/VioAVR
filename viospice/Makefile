# Makefile for VioAVR XSpice Code Model (Bridge Client)

CXX = g++
CXXFLAGS = -fPIC -O3 -std=c++17 -I../include -I/usr/include/ngspice
LDFLAGS = -shared -fPIC -lrt -lpthread

OBJECTS = viospice_bridge.o ../src/core/bridge_shm_client.o
TARGET = vioavr.cm

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
