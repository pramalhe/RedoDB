/* Author: https://arxiv.org/pdf/1806.04780.pdf */
#ifndef UTILITIES_H_
#define UTILITIES_H_

#include <climits>				//for max int
#include <fstream>
#include <iostream>
#include <malloc.h>
#include <libvmmalloc.h>

#define MAX_THREADS 144         // upper bound on the number of threads in your system
#define PADDING 512             // Padding must be multiple of 4 for proper alignment
#define QUEUE_SIZE 1000000      // Initial size of the queue
#define CAS __sync_bool_compare_and_swap
#define MFENCE __sync_synchronize

std::ofstream file;

#ifdef MEASURE_PWB
extern thread_local uint64_t tl_num_pwbs;
extern thread_local uint64_t tl_num_pfences;
#endif


void FLUSH(void *p)
{
#ifdef MEASURE_PWB
    tl_num_pwbs++;
#endif
#ifdef PWB_IS_CLFLUSH
    asm volatile ("clflush (%0)" :: "r"(p));
#elif PWB_IS_CLFLUSHOPT
    asm volatile(".byte 0x66; clflush %0" : "+m" (*(volatile char *)(p)));    // clflushopt (Kaby Lake)
#elif PWB_IS_CLWB
    asm volatile(".byte 0x66; xsaveopt %0" : "+m" (*(volatile char *)(p)));  // clwb() only for Ice Lake onwards
#else
#error "You must define what PWB is. Choose PWB_IS_CLFLUSH if you don't know what your CPU is capable of"
#endif
}

void FLUSH(void volatile * p)
{
#ifdef MEASURE_PWB
    tl_num_pwbs++;
#endif
#ifdef PWB_IS_CLFLUSH
    asm volatile ("clflush (%0)" :: "r"(p));
#elif PWB_IS_CLFLUSHOPT
    asm volatile(".byte 0x66; clflush %0" : "+m" (*(volatile char *)(p)));    // clflushopt (Kaby Lake)
#elif PWB_IS_CLWB
    asm volatile(".byte 0x66; xsaveopt %0" : "+m" (*(volatile char *)(p)));  // clwb() only for Ice Lake onwards
#else
#error "You must define what PWB is. Choose PWB_IS_CLFLUSH if you don't know what your CPU is capable of"
#endif
}

void SFENCE()
{
#ifdef MEASURE_PWB
    tl_num_pfences++;
#endif
    asm volatile ("sfence" ::: "memory");
}

#ifdef PWB_IS_CLFLUSH
    #define FENCE MFENCE
#elif PWB_IS_CLFLUSHOPT
    #define FENCE SFENCE
#elif PWB_IS_CLWB
    #define FENCE SFENCE
#else
#error "You must define what PWB is. Choose PWB_IS_CLFLUSH if you don't know what your CPU is capable of"
#endif

#define BARRIER(p) {FLUSH(p);FENCE();}
#define OPT_BARRIER(p) {FLUSH(p);}

#ifdef MANUAL_FLUSH
	#define MANUAL(x) x
#else
	#define MANUAL(x) {}
#endif

#ifdef READ_WRITE_FLUSH
	#define RFLUSH(x) x
	#define WFLUSH(x) x
#else
	#ifdef WRITE_FLUSH
		#define RFLUSH(x) {}
		#define WFLUSH(x) x
	#else
		#define RFLUSH(x) {}
		#define WFLUSH(x) {}
	#endif
#endif


#endif /* UTILITIES_H_ */
