make: MemoryManager.cpp
	g++ -c MemoryManager.cpp -o MemoryManager.o
	ar cr libMemoryManager.a MemoryManager.o