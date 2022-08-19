#include <stdlib.h>
#include<iostream>
#include <string.h>
#include "ChunkAlloc.hpp"
#include <sys/mman.h>
#include"pin_tags.h"

static size_t allocated_chunks = 0;

void init_chunk() {
	// This creates an tag that covers no memory, since the max is very small and min is very large.
	// We'll grow it below.
	TAG_START("chunks", reinterpret_cast<void*>(-1), reinterpret_cast<void*>(0), true);   
}

void * alloc_chunk() { // allocate CHUNK_SIZE bytes of memory by asking the operating system for it.
	
	// this is actually malloc gets it's memory from the kernel.
	// mmap() can do many things.  In this case, it just asks the kernel to
	// give us some pages of memory.  They are guaranteed to contain zeros.
	void * r = mmap(NULL, CHUNK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANONYMOUS, 0, 0); 
	if (r == MAP_FAILED) { 
		std::cerr << "alloc_chunk() failed. This often means you've allocated too many chunks. Exiting: " << strerror(errno) << "\n";
		exit(1);
	}
	TAG_GROW("chunks", r, reinterpret_cast<uint8_t*>(r) + CHUNK_SIZE);
	allocated_chunks++; // This is just statistics tracking
	return r;
}

void free_chunk(void*p) { // Return the chunk to the OS.  After this, accesses to the addresses in the chunk will result in SEGFAULT
	int r = munmap(p, CHUNK_SIZE);
	if (r != 0) {
		std::cerr << "free_chunk() failed. exiting: " << strerror(errno) << "\n";
		exit(1);
	}
	allocated_chunks--;
}

size_t get_allocated_chunks() {
	return allocated_chunks;
}
