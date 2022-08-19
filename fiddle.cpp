#include <cstdlib>
#include "archlab.hpp"
#include <unistd.h>
#include"pin_tags.h"
#include"omp.h"
#include"papi.h"
#include<algorithm>
#include<cstdint>
#include"function_map.hpp"

#include <dlfcn.h>


uint array_size;

class alloc_test: public benchmark_env<uint64_t*(*)(uint64_t, uint64_t)> {
public:
	void reset_environment(const parameter_map_t & parameters) {}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t count = boost::any_cast<uint64_t>(parameters["size"]);
		uint64_t seed= boost::any_cast<uint64_t>(parameters["arg1"]);
		auto f = cast_function(the_func);
		return [f, seed, count]() {
			f(count, seed);
		};
	}
};

class one_array: public benchmark_env<uint64_t*(*)(uint64_t *, unsigned long int)> {
	uint64_t * array;
	uint64_t max_size;
public:
	one_array(uint64_t max_size): max_size(max_size) {
		array = new uint64_t[max_size];
	}
	
	void reset_environment(const parameter_map_t & parameters) {
		uint64_t a = 1;
		for (unsigned int i = 0; i < max_size; i++) {
			array[i] = fast_rand(&a);
		}
	}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t size = boost::any_cast<uint64_t>(parameters["size"]);
		auto f = cast_function(the_func);
		uint64_t * _array = array;
		return [f, _array, size]() {
			       f(_array, size);
		       };
	}
	~one_array() {
		delete [] array;
	}
};

class two_arrays: public benchmark_env<uint64_t*(*)(uint64_t *, unsigned long int,uint64_t *, unsigned long int)> {
	uint64_t * array1;
	uint64_t * array2;
	uint64_t max_size;
public:
	two_arrays(uint64_t max_size): max_size(max_size) {
		array1 = new uint64_t[max_size];
		array2 = new uint64_t[max_size];
	}
	
	void reset_environment(const parameter_map_t & parameters) {
		uint64_t a = 1;
		for (unsigned int i = 0; i < max_size; i++) {
			array1[i] = fast_rand(&a);
			array2[i] = fast_rand(&a);
		}
	}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t size = boost::any_cast<uint64_t>(parameters["size"]);
		uint64_t size2 = boost::any_cast<uint64_t>(parameters["size2"]);
		auto f = cast_function(the_func);
		uint64_t * _array1 = array1;
		uint64_t * _array2 = array2;
		return [f, _array1,_array2, size, size2]() {
			f(_array1, size, _array2, size2);
		};
	}
	~two_arrays() {
		delete [] array1;
		delete [] array2;
	}
};

class three_arrays: public benchmark_env<uint64_t*(*)(uint64_t *, unsigned long int,
						      uint64_t *, unsigned long int,
						      uint64_t *, unsigned long int)> {
	uint64_t * array1;
	uint64_t * array2;
	uint64_t * array3;
	uint64_t max_size;
public:
	three_arrays(uint64_t max_size): max_size(max_size) {
		array1 = new uint64_t[max_size];
		array2 = new uint64_t[max_size];
		array3 = new uint64_t[max_size];
	}
	
	void reset_environment(const parameter_map_t & parameters) {
		uint64_t a = 1;
		for (unsigned int i = 0; i < max_size; i++) {
			array1[i] = fast_rand(&a);
			array2[i] = fast_rand(&a);
			array3[i] = fast_rand(&a);
		}
	}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t size = boost::any_cast<uint64_t>(parameters["size"]);
		uint64_t size2 = boost::any_cast<uint64_t>(parameters["size2"]);
		uint64_t size3 = boost::any_cast<uint64_t>(parameters["size3"]);
		auto f = cast_function(the_func);
		uint64_t * _array1 = array1;
		uint64_t * _array2 = array2;
		uint64_t * _array3 = array3;
		return [f, _array1,_array2, _array3, size, size2, size3]() {
			f(_array1, size, _array2, size2, _array3, size3);
		};
	}
	
	~three_arrays() {
		delete [] array1;
		delete [] array2;
		delete [] array3;
	}
};

class one_array_1arg: public benchmark_env<uint64_t*(*)(uint64_t *, unsigned long int, unsigned long int)> {
	uint64_t * array;
	uint64_t max_size;
public:
	one_array_1arg(uint64_t max_size): max_size(max_size) {
		array = new uint64_t[max_size];
	}
	
	void reset_environment(const parameter_map_t & parameters) {
		// uint64_t a = 1;
		// for (unsigned int i = 0; i < max_size; i++) {
		// 	array[i] = fast_rand(&a);
		// }
	}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		assert(parameters.find("size") != parameters.end());
		assert(parameters.find("arg1") != parameters.end());
		uint64_t size = boost::any_cast<uint64_t>(parameters["size"]);
		uint64_t arg1 = boost::any_cast<uint64_t>(parameters["arg1"]);
		auto f = cast_function(the_func);
		uint64_t * _array = array;
		return [f, _array, size, arg1]() {
			       f(_array, size, arg1);
		       };
	}
	~one_array_1arg() {
		delete [] array;
	}
};



class raw_bytes: public benchmark_env<uint64_t*(*)(uint64_t *, unsigned long int)> {
	uint8_t * array;
	uint64_t max_size;
public:
	raw_bytes(uint64_t max_size): max_size(max_size) {
		array =  new uint8_t[max_size];
	}
	
	void reset_environment(const parameter_map_t & parameters) {
		uint64_t a = 1;
		uint64_t * t = reinterpret_cast<uint64_t*>(array);
		
		for (unsigned int i = 0; i < max_size/sizeof(uint64_t); i++) {
			t[i] = fast_rand(&a);
		}
	}
	
	std::function<void()> get_function(void * the_func, parameter_map_t & parameters) {
		uint64_t size = boost::any_cast<uint64_t>(parameters["size"]);
		auto f = cast_function(the_func);
		uint64_t * _array = reinterpret_cast<uint64_t*>(array);
		return [f, _array, size]() {
			       f(_array, size);
		       };
	}
	~raw_bytes() {
		delete [] array;
	}
};

int main(int argc, char *argv[])
{

	
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

	std::vector<unsigned long int> sizes2;
	std::vector<unsigned long int> default_sizes2;
	default_sizes2.push_back(1024*1024);
	archlab_add_multi_option<std::vector<unsigned long int> >("size2",
					      sizes2,
					      default_sizes2,
					      "1024*1024",
					      "Size2.  Pass multiple values to run with multiple sizes.");
	std::vector<unsigned long int> sizes3;
	std::vector<unsigned long int> default_sizes3;
	default_sizes3.push_back(1024*1024);
	archlab_add_multi_option<std::vector<unsigned long int> >("size3",
					      sizes3,
					      default_sizes3,
					      "1024*1024",
					      "Size3.  Pass multiple values to run with multiple sizes.");

	std::vector<unsigned long int> arg1s;
	std::vector<unsigned long int> default_arg1s;
	default_arg1s.push_back(1);
	archlab_add_multi_option<std::vector<unsigned long int> >("arg1",
					      arg1s,
					      default_arg1s,
					      "1",
					      "Arg1.  Pass multiple values to run with multiple arg1s.");

	unsigned long int reps;
	archlab_add_option<unsigned long int >("reps",
					       reps,
					       1,
					       "1",
					       "How many times to repeat the experiment.");

	bool tag_functions;
	archlab_add_option<bool >("tag-functions",
				  tag_functions,
				  true,
				  "true",
				  "Add tags for each function invoked");

	unsigned long int iterations;
	archlab_add_option<unsigned long int >("iters",
					       iterations,
					       1,
					       "1",
					       "How many times to run the function.");

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

       
	REGISTER_ENV(one_array_1arg, new one_array_1arg(*std::max_element(sizes.begin(), sizes.end())));
	REGISTER_ENV(one_array, new one_array(*std::max_element(sizes.begin(), sizes.end())));
	REGISTER_ENV(two_arrays, new two_arrays(*std::max_element(sizes.begin(), sizes.end())));
	REGISTER_ENV(three_arrays, new three_arrays(*std::max_element(sizes.begin(), sizes.end())));
	REGISTER_ENV(raw_bytes, new raw_bytes(*std::max_element(sizes.begin(), sizes.end())));
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
		for(auto & arg1: arg1s ) {
			params["arg1"] = arg1;
			for(auto & size: sizes ) {
				params["size"] = size;
				for(auto & size2: sizes2 ) {
					params["size2"] = size2;
					for(auto & size3: sizes3 ) {
						params["size3"] = size3;
						for(auto & function : functions) {
							START_TRACE();
							std::cerr << "Running " << function;
							function_spec_t f_spec = function_map::get()[function];
							auto env = benchmark_env_map::get()[f_spec.first];
							auto fut = env->get_function(f_spec.second, params);
							//NEW_TRACE(function.c_str());
							for(uint r = 0; r < reps; r++) {
								env->reset_environment(params);
								std::cerr << ".";
								//pristine_machine();					
								theDataCollector->register_tag("size2",size2);
								theDataCollector->register_tag("size3",size3);
								theDataCollector->register_tag("size",size);
								theDataCollector->register_tag("arg1",arg1);
							
								{								
									ArchLabTimer timer;					
									flush_caches();
									theDataCollector->disable_prefetcher();
									timer.attr("function", function.c_str());
									timer.attr("cmdlineMHz", mhz);
									timer.attr("rep", r);
									timer.attr("iterations", iterations);
									timer.go();
									if (tag_functions) DUMP_START_ALL(function.c_str(), false);
									for(unsigned int i =0; i < iterations; i++) {
										fut();
									}
									if (tag_functions )DUMP_STOP(function.c_str());
								}								
							}
							std::cerr << "\n";
						}
					}
				}
			}
		}
	}
	archlab_write_stats();
	return 0;
}
