CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++17 -O2 -g
LDFLAGS = -pthread

all: dns

dns: src/main.cpp src/DNSResolver.cpp src/DNSProtocol.cpp src/CLIParser.cpp
	$(CXX) $(CXXFLAGS) -o dns src/*.cpp $(LDFLAGS)

clean:
	rm -f dns
	rm -rf obj

test: dns
	python3 simple_test.py

.PHONY: all clean test