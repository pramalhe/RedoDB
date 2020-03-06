/*
 * Copyright 2017-2020
 *   Andreia Correia <andreia.veiga@unine.ch>
 *   Pedro Ramalhete <pramalhe@gmail.com>
 *   Pascal Felber <pascal.felber@unine.ch>
 *
 * This work is published under the MIT license. See LICENSE.txt
 */

#include "../cxpuc/CXPUC.hpp"


namespace cxpuc {

// Global with the 'main' size. Used by pload()
uint64_t g_main_size = 0;
// Global with the 'main' addr. Used by pload()
uint8_t* g_main_addr = 0;
uint8_t* g_main_addr_end;
uint8_t* g_region_end;

// Counter of nested write transactions
thread_local int64_t tl_nested_write_trans = 0;
// Counter of nested read-only transactions
thread_local int64_t tl_nested_read_trans = 0;
// Indicates the combined instance that is being used by the current transaction.
// This is used by static methods like: get_root_obj(), alloc(), free().
thread_local int tl_icomb = -1;
// Used by static methods like alloc/free
thread_local EsLoco<persist>* tl_esloco = nullptr;

} // End of cxpuc namespace
