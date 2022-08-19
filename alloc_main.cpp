#include <cstdlib>
#include "archlab.hpp"
#include <unistd.h>
#include"pin_tags.h"
#include"omp.h"
#include"papi.h"
#include<algorithm>
#include<cstdint>
#include"function_map.hpp"
#include"ChunkAlloc.hpp"
#include <dlfcn.h>


uint array_size;

class alloc_test: public benchmark_env<uint64_t*(*)(uint64_t, uint64_t)> {
public:
	void reset_environment(const parameter_map_t & parameters) {}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t count = boost::any_cast<uint64_t>(parameters["size"]);
		uint64_t seed= boost::any_cast<uint64_t>(parameters["seed"]);
		auto f = cast_function(the_func);
		return [f, seed, count]() {
			f(count, seed);
		};
	}
};

int main(int argc, char *argv[])
{

	init_chunk();
	
	std::vector<int> mhz_s;
	std::vector<int> default_mhz;
	load_frequencies();
	default_mhz.push_back(cpu_frequencies_array[1]);
	std::stringstream clocks;
	for(int i =0; cpu_frequencies_array[i] != 0; i++) {
		clocks << cpu_frequencies_array[i] << " ";
	}
	std::stringstream fastest;
	fastest << cpu_frequencies_array[0];

	archlab_add_multi_option<std::vector<int> >("MHz",
						    mhz_s,
						    default_mhz,
						    fastest.str(),
						    "Which clock rate to run.  Possibilities on this machine are: " + clocks.str());

	std::vector<std::string> functions;
	std::vector<std::string> default_functions;
	// std::stringstream available_functions;
	// for(auto & f: function_map::get()) {
	// 	available_functions << "'" << f.first << "' ";
	// }
	
	default_functions.push_back("ALL");
	archlab_add_multi_option<std::vector<std::string>>("function",
							   functions,
							   default_functions,
							   "ALL",
							   "Which functions to run.");

	std::vector<std::string> libs;
	std::vector<std::string> default_libs;
	archlab_add_multi_option<std::vector<std::string>>("lib",
							   libs,
							   default_libs,
							   "ALL",
							   "Libraries to load");
		
	std::vector<unsigned long int> sizes;
	std::vector<unsigned long int> default_sizes;
	default_sizes.push_back(1024*1024);
	archlab_add_multi_option<std::vector<unsigned long int> >("size",
					      sizes,
					      default_sizes,
					      "1024*1024",
					      "Size.  Pass multiple values to run with multiple sizes.");


	std::vector<unsigned long int> seeds;
	std::vector<unsigned long int> default_seeds;
	default_seeds.push_back(0xdeadbeef);
	archlab_add_multi_option<std::vector<unsigned long int> >("seed",
					      seeds,
					      default_seeds,
					      "0xdeadbeef",
					      "Seed.  Pass multiple values to run with multiple seeds.");

	bool tag_functions;
	archlab_add_option<bool >("tag-functions",
				  tag_functions,
				  true,
				  "true",
				  "Add tags for each function invoked");


	archlab_parse_cmd_line(&argc, argv);


	for(auto & l: libs) {
		void * lib = dlopen(l.c_str(), RTLD_LOCAL|RTLD_NOW);
		if (lib == NULL){
			std::cerr << "Couldn't loadlib " << dlerror() << "\n";
			exit(1);
		}
		void (*f)(function_map_t &) = (void (*)(function_map_t &))dlsym(lib, "register_functions");
		if (f == NULL) {
			std::cerr << "Couldn't load function " << dlerror() << "\n";
			exit(1);
		}
		f(function_map::get());
	}

	REGISTER_ENV(alloc_test, new alloc_test);

	theDataCollector->disable_prefetcher();

	if (std::find(functions.begin(), functions.end(), "ALL") != functions.end()) {
		functions.clear();
		for(auto & f : function_map::get()) {
			functions.push_back(f.first);
		}
	}
	
	for(auto & function : functions) {
		auto t= function_map::get().find(function);
		if (t == function_map::get().end()) {
			std::cerr << "Unknown function: " << function <<"\n";
			exit(1);
		}
		auto s = benchmark_env_map::get().find(t->second.first);
		if ( s == benchmark_env_map::get().end()) {
			std::cerr << "Unknown benchmark env: " << t->second.first << " for " << function <<"\n";
			exit(1);
		}
	}
	
	parameter_map_t params;
	for(auto &mhz: mhz_s) {
		set_cpu_clock_frequency(mhz);
		for(auto & seed: seeds ) {
			params["seed"] = seed;
			for(auto & size: sizes ) {
				params["size"] = size;
				for(auto & function : functions) {
					START_TRACE();
					std::cerr << "Running " << function;
					function_spec_t f_spec = function_map::get()[function];
					auto env = benchmark_env_map::get()[f_spec.first];
					auto fut = env->get_function(f_spec.second, params);
					//NEW_TRACE(function.c_str());
					//pristine_machine();					
					theDataCollector->register_tag("size",size);
					theDataCollector->register_tag("seed",seed);
					theDataCollector->register_tag("function", function.c_str());
					theDataCollector->register_tag("cmdlineMHz", mhz);
					theDataCollector->disable_prefetcher();
					flush_caches();
					
					{
						if (tag_functions) DUMP_START_ALL(function.c_str(), false);
						fut();
						if (tag_functions ) DUMP_STOP(function.c_str());
					}								
					std::cerr << "\n";
				}
			}
		}
	}
	
	archlab_write_stats();
	return 0;
}
