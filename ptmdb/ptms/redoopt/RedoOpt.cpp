/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */
#include "../redoopt/RedoOpt.hpp"


namespace redoopt {

// Global with the 'main' size. Used by pload()
uint64_t g_main_size = 0;
uint8_t* g_main_addr = 0;
uint8_t* g_main_addr_end;
uint8_t* g_region_end;

// Counter of nested write transactions
thread_local int64_t tl_nested_write_trans = 0;
// Counter of nested read-only transactions
thread_local int64_t tl_nested_read_trans = 0;
// Global instance
RedoOpt gRedo {};

thread_local varLocal tlocal;

#ifdef MEASURE_PWB
// Used only if MEASURE_PWB is defined
thread_local uint64_t tl_num_pwbs = 0;
thread_local uint64_t tl_num_pfences = 0;
#endif

} // End of redotimedhash namespace
