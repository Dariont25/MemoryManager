#pragma once

#include <string>
#include <functional>
#include <vector>
#include <iterator>
#include <map>
#include <iostream>
#include <string>
#include <cstring>
#include <iostream>
#include <bitset>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>

struct Node {
	//length is in units of words!

	bool isFree = true;
	int length;
	int index;

	Node(int index, int length, bool isFree);
};

class MemoryManager {
	std::vector<Node> nodes;
	std::vector<uint64_t> memoryBlock;
	std::map<int, int> indextoNode;
	uint64_t* memStart;
	unsigned wordSize;
	size_t byteLimit;
	std::function<int(int, void*)> algorithm;
public:
	MemoryManager(unsigned wordSize, std::function<int(int, void*)> allocator);
	~MemoryManager();
	void initialize(size_t sizeInWords);
	void shutdown();
	void* allocate(size_t sizeInBytes);
	void free(void* address);
	void setAllocator(std::function<int(int, void*)> allocator);
	int dumpMemoryMap(char* filename);
	void* getList();
	void* getBitmap();
	unsigned getWordSize();
	void* getMemoryStart();
	unsigned getMemoryLimit();
};

int bestFit(int sizeInWords, void* list);
int worstFit(int sizeInWords, void* list);

