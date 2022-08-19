
#include <stdlib.h>
#include<iostream>
#include<archlab.hpp>
#include"function_map.hpp"
#include<map>
#include"ReferenceAllocator.hpp"
#include"SolutionAllocator.hpp"
#include"pin_tags.h"

template<class Allocator>
void exercise(Allocator * allocator, size_t count, int iterations, uint64_t seed, bool cleanup = false) {
	// Interesting allocator behaviors an bugs emerge when the allocator
	// has to allocate and free objects in complex patterns.
	//
	// To simulate that, we allocate count items and then, on each
	// iteration, free about 1/4 of them and replace them with new items.
	
	std::vector<typename Allocator::ItemType *> items(count);
	TAG_START_ALL("exercise", true);
	
	for(unsigned int i = 0; i < count; i++)
		items[i] = NULL;

	for(int i = 0; i < iterations; i++) {
		for(unsigned int j = 0; j < count; j++) {
			if (items[j] == NULL) {
				items[j] = allocator->alloc();
			}
		}
		for(unsigned int j = 0; j < count; j++) {
			fast_rand(&seed);
			if (seed & 0x3) {
				allocator->free(items[j]);
				items[j] = NULL;
			}
		}
	}
	if (cleanup) {
		for(unsigned int j = 0; j < count; j++) {
			if (items[j]) {
				allocator->free(items[j]);
				items[j] = NULL;
			}
		}
	}

	TAG_STOP("exercise");
}


template<class Allocator>
void bench(uint64_t count, uint64_t seed, bool do_exercise) {
	{
		auto alloc = new Allocator;
		if (do_exercise){ // warm it up.
			exercise<Allocator>(alloc, 4000, 20, seed);
		}
		ArchLabTimer timer;					
		timer.attr("count", count);
		timer.attr("seed", seed);
		timer.attr("test", "exercise");
		timer.attr("exercise", do_exercise);
		timer.go();
		exercise<Allocator>(alloc, count/16, 16, seed);
		delete alloc;
	}
}

template<class Allocator>
void microbench(uint64_t count, uint64_t seed, bool do_exercise) {

	auto alloc = new Allocator;
	
	if (do_exercise) { // get the allocator warmed up.
		exercise<Allocator>(alloc, 4000, 20, seed);
	}
	std::vector<typename Allocator::ItemType*> items(count);
	{
		ArchLabTimer timer;					
		timer.attr("count", count);  // These show up in the csv file.
		timer.attr("seed", seed);
		timer.attr("test", "alloc");
		timer.attr("exercise", do_exercise);
		timer.go(); // start timing here
		for(uint64_t i = 0; i < count; i++) {
			items[i] = alloc->alloc();
		}
	}

	if (do_exercise) {
		exercise<Allocator>(alloc, 4000, 20, seed);
	}
	{
		ArchLabTimer timer;					
		timer.attr("count", count);
		timer.attr("seed", seed);
		timer.attr("test", "free");
		timer.attr("exercise", do_exercise);
		timer.go();
		for(uint64_t i = 0; i < count; i++) {
			alloc->free(items[i]);
		}
	} 
	delete alloc;

}

//BEGIN
struct MissingLink {
	struct MissingLink * next;
};

extern "C"
struct MissingLink*  __attribute__((noinline)) do_misses(struct MissingLink * l) {
	for(uint i = 0; i < 10000000; i++) {
		l = l->next;
	}
	return l;
}

template<class Allocator>
uint64_t  miss_machine(uint64_t count, uint64_t seed) {
	auto alloc = new Allocator; // create the allocator.

	STOP_TRACE();
	exercise<Allocator>(alloc, 10000, 20, seed); // warm it up.
	START_TRACE();


	TAG_START_ALL("build_miss_machine", true); // For Moneta tracing

	std::vector<struct MissingLink *> links(count);  // Storage for the links
	for(auto &i : links) {                           // allocate them.
		i = alloc->alloc();
		i->next = NULL;
	}
	
	std::shuffle(links.begin(), links.end(), fast_URBG(seed));  // randomize the order of the links
	for(uint i = 0; i < links.size() -1; i++) { 
		links[i]->next = links[i+1];           // Make the next pointers reflect the ordering.
	}
	links.back()->next = links.front(); // complete the circle

	struct MissingLink * l = links[0];
	{
		ArchLabTimer timer;					
		timer.attr("count", count); // record data about experiment
		timer.attr("seed", seed);
		timer.attr("test", "miss_machine");
		timer.go();

		TAG_STOP("build_miss_machine");// For Moneta tracing
		
		TAG_START_ALL("miss_machine", true); // for moneta tracing
		l = do_misses(l);        // Do the misses.  Put it in a function so we can easily look at the CFG.
		TAG_STOP("miss_machine"); // for moneta tracing
	}
	return reinterpret_cast<uintptr_t>(l); // Return something  depending on do_misses() to prevent the compiler from optimizing it away.
}
//END


template<class Allocator>
void run_bench(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("bytes", sizeof(typename Allocator::ItemType));
	theDataCollector->register_tag("alignment", Allocator::Alignment);		
	bench<Allocator>(count, seed, true);	
}

template<class Allocator>
void run_miss_machine(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("test", "run_miss_machine");
	theDataCollector->register_tag("bytes", sizeof(typename Allocator::ItemType));
	theDataCollector->register_tag("alignment", Allocator::Alignment);		
	miss_machine<Allocator>(count, seed);	
}

template<class Allocator>
void run_microbench(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("bytes", sizeof(typename Allocator::ItemType));
	theDataCollector->register_tag("alignment", Allocator::Alignment);		
	microbench<Allocator>(count, seed, true);	
}



// Starter functions

extern "C"
uint64_t * allocator_bench_starter(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "ReferenceAllocator");
	run_bench<ReferenceAllocator<uint8_t[3], 16>>(count, seed);
	run_bench<ReferenceAllocator<uint8_t[125], 32>>(count, seed);
	run_bench<ReferenceAllocator<uint8_t[4096], 4096>>(count, seed);
	return NULL;
}
FUNCTION(alloc_test, allocator_bench_starter); // this let's you pass this function as an argument to `--function` on the command line.


extern "C"
uint64_t * allocator_microbench_starter(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "ReferenceAllocator");
	run_microbench<ReferenceAllocator<uint[4], 8>>(count, seed);
	run_microbench<ReferenceAllocator<uint[1024], 4096>>(count, seed);
	return NULL;
}
FUNCTION(alloc_test, allocator_microbench_starter);


extern "C"
uint64_t * miss_machine_starter(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "ReferenceAllocator");
	run_miss_machine<ReferenceAllocator<struct MissingLink, sizeof(struct MissingLink)>>(count, seed);
	return NULL;
}
FUNCTION(alloc_test, miss_machine_starter);


// Solution functions


extern "C"
uint64_t * allocator_bench_solution(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "SolutionAllocator");
	
	run_bench<SolutionAllocator<uint8_t[3], 16>>(count, seed);
	run_bench<SolutionAllocator<uint8_t[125], 32>>(count, seed);
	run_bench<SolutionAllocator<uint8_t[4096], 4096>>(count, seed);
	
	return NULL;
}
FUNCTION(alloc_test, allocator_bench_solution);

extern "C"
uint64_t * allocator_microbench_solution(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "SolutionAllocator");
	run_microbench<SolutionAllocator<uint[4], 8>>(count, seed);
	run_microbench<SolutionAllocator<uint[1024], 4096>>(count, seed);
	return NULL;
}
FUNCTION(alloc_test, allocator_microbench_solution);

extern "C"
uint64_t * miss_machine_solution(uint64_t count, uint64_t seed) {
	theDataCollector->register_tag("impl", "SolutionAllocator");
	run_miss_machine<SolutionAllocator<struct MissingLink, sizeof(struct MissingLink)>>(count, seed);
	return NULL;
}
FUNCTION(alloc_test, miss_machine_solution);


