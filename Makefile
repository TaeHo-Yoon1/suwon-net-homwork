CXX = g++
CXXFLAGS = -std=c++11 -pthread

all: server client

server: server/main.cpp
	$(CXX) $(CXXFLAGS) -o server server/main.cpp

client: client/main.cpp
	$(CXX) $(CXXFLAGS) -o client client/main.cpp

clean:
	rm -f server client 