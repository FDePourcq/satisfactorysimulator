
all: debug

debug:
	g++ -g -O1 test.cpp murmur3.c  -o factorysimulator

release:
	g++ -g -O3 test.cpp murmur3.c  -o factorysimulator
