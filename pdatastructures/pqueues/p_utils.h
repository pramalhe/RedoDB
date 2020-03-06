/* Author: https://arxiv.org/pdf/1806.04780.pdf */
#ifndef P_UTILS_H_
#define P_UTILS_H_

#include "Utilities.h"
#include <atomic>
#include <cstdint>

namespace utils {
	// The conditional should be removed by the compiler
	// this should work with pointer types, or pairs of integers
	template <class ET>
	inline bool CAS(ET volatile *ptr, ET oldv, ET newv) { 
#ifdef MEASURE_PWB
    tl_num_pfences++;
#endif
	  bool ret;
	  if (sizeof(ET) == 1) { 
		ret = __sync_bool_compare_and_swap_1((bool*) ptr, *((bool*) &oldv), *((bool*) &newv));
	  } else if (sizeof(ET) == 8) {
		ret = __sync_bool_compare_and_swap_8((long*) ptr, *((long*) &oldv), *((long*) &newv));
		//return utils::LCAS((long*) ptr, *((long*) &oldv), *((long*) &newv));
	  } else if (sizeof(ET) == 4) {
		ret = __sync_bool_compare_and_swap_4((int *) ptr, *((int *) &oldv), *((int *) &newv));
		//return utils::SCAS((int *) ptr, *((int *) &oldv), *((int *) &newv));
	  } 
	#if defined(MCX16)
	  else if (sizeof(ET) == 16) {
		ret = __sync_bool_compare_and_swap_16((__int128*) ptr,*((__int128*)&oldv),*((__int128*)&newv));
	  }
	#endif
	  else {
		std::cout << "CAS bad length (" << sizeof(ET) << ")" << std::endl;
		abort();
	  }
	  WFLUSH(BARRIER(ptr););
	  return ret;
	}

	template <class ET>
	inline bool CAS_NO_BARRIER(ET *ptr, ET oldv, ET newv) { 
#ifdef MEASURE_PWB
    tl_num_pfences++;
#endif
	  if (sizeof(ET) == 1) { 
		return __sync_bool_compare_and_swap_1((bool*) ptr, *((bool*) &oldv), *((bool*) &newv));
	  } else if (sizeof(ET) == 8) {
		return __sync_bool_compare_and_swap_8((long*) ptr, *((long*) &oldv), *((long*) &newv));
	  } else if (sizeof(ET) == 4) {
		return __sync_bool_compare_and_swap_4((int *) ptr, *((int *) &oldv), *((int *) &newv));
	  } 
	#if defined(MCX16)
	  else if (sizeof(ET) == 16) {
		return __sync_bool_compare_and_swap_16((__int128*) ptr,*((__int128*)&oldv),*((__int128*)&newv));
	  }
	#endif
	  else {
		std::cout << "CAS bad length (" << sizeof(ET) << ")" << std::endl;
		abort();
	  }
	}
	
	template <class ET>
	inline ET READ(ET &ptr)
	{
		ET ret = ptr;
		RFLUSH(BARRIER(&ptr));
		return ret;
	}

	template <class ET>
	inline void WRITE(ET &ptr, ET val)
	{
		ptr = val;
		WFLUSH(BARRIER(&ptr));
	}	
}

struct pc_seq_cur {
	void* pc;
	uint32_t seq_num;
	uint32_t cur;
};

class alignas(128) closure {
    public:
		void* pc[2];
    	void* data[2][2];
		uint64_t cur_seq;

        closure() {
		pc[0] = pc[1] = 0;
		cur_seq = 0ULL;
        	for(int i = 0; i < 2; i++)
        		for(int j = 0; j < 2; j++)
        			data[i][j] = NULL;
        }
};

static const uint64_t OPID_MASK = (((uint64_t) 1) << 32)-1;
static const uint64_t TID_MASK = ((((uint64_t) 1) << 32)-1) << 32;
static const uint32_t SHIFT = (1 << 30);
#define COMBINE(x, y) (((uint64_t(x)) << 32) + (uint64_t(y)))
#define TID(x) ((uint32_t)(((x) & TID_MASK) >> 32))
#define OPID(x) ((uint32_t) ((x) & OPID_MASK))


static closure closures[MAX_THREADS];
static closure closures2[MAX_THREADS];

uint32_t get_capsule_number(int my_id)
{
	return closures[my_id].cur_seq >> 32;
}

void capsule_boundary_opt(int my_id) {
	uint64_t cur = 1ULL- (closures[my_id].cur_seq & 1ULL);
	closures[my_id].pc[cur] = __builtin_return_address(0);
	closures[my_id].cur_seq = (closures[my_id].cur_seq) + SHIFT + cur;
	FLUSH(&closures[my_id]);	// In all other cases, this barrier
    // is part of the previous utils::CAS	
}

void capsule_boundary_opt(int my_id, void* a, void* b) {
	uint64_t cur = 1ULL- (closures[my_id].cur_seq & 1ULL);
	closures[my_id].data[0][cur] = a;
	closures[my_id].data[1][cur] = b;
	closures[my_id].pc[cur] = __builtin_return_address(0);
	closures[my_id].cur_seq = (closures[my_id].cur_seq) + SHIFT + cur;
	FLUSH(&closures[my_id]);	// In all other cases, this barrier
	// is part of the previous utils::CAS	
}

void capsule_boundary_opt(int my_id, void* a, void* b, void* c) {
	uint64_t cur = 1ULL- (closures[my_id].cur_seq & 1ULL);
	closures2[my_id].data[0][cur] = c;
	BARRIER(&closures2[my_id]);
	closures[my_id].data[0][cur] = a;
	closures[my_id].data[1][cur] = b;
	closures[my_id].pc[cur] = __builtin_return_address(0);
	closures[my_id].cur_seq = (closures[my_id].cur_seq) + SHIFT + cur;
	FLUSH(&closures[my_id]);	// In all other cases, this barrier
	// is part of the previous utils::CAS	
}

template <class PT>
struct RCas{
	alignas(16) PT ptr;
	uint64_t id;
};


static uint32_t rcas_ann[MAX_THREADS][MAX_THREADS + PADDING] = { { 0 } };

template<class PT>
void rcas_init(RCas<PT> volatile & loc, PT ptr, uint64_t ids) {
	loc.ptr = ptr;
	loc.id = ids;
}

template<class PT>
void rcas_init(RCas<PT> volatile & loc, PT ptr, uint32_t tid, uint32_t opid) {
	rcas_init(loc, ptr, COMBINE(tid, opid));
}

template<class PT>
void rcas_init(RCas<PT> volatile & loc, PT ptr) {
	rcas_init(loc, ptr, COMBINE(MAX_THREADS, 0));
}

template<class PT>
void rcas_init(RCas<PT> volatile & loc) {
	rcas_init(loc, (PT) NULL, COMBINE(MAX_THREADS, 0));
}

template<class PT>
PT rcas_read(RCas<PT> volatile & loc) {
	return loc.ptr;
}

template<class PT>
bool rcas_cas(RCas<PT> volatile * loc, PT exp_ptr, PT new_ptr) {
	return rcas_cas(loc, exp_ptr, new_ptr, MAX_THREADS, 0);
}

template<class PT>
bool rcas_cas(RCas<PT> volatile * loc, PT exp_ptr, PT new_ptr, uint32_t tID, uint32_t opID) {
	RCas<PT> x;
	x.ptr = loc->ptr;
	x.id = loc->id;
	RFLUSH(BARRIER(loc));
	if(x.ptr != exp_ptr) return 0;
	uint32_t old_tID = TID(x.id);
	uint32_t old_opID = OPID(x.id);
	if(old_tID < MAX_THREADS && rcas_ann[tID][old_tID] < old_opID) // the read of rcas_ann[tID][old_tID] can be ld rather than ld_acq
	{
		MANUAL(BARRIER(loc));
		utils::WRITE(rcas_ann[tID][old_tID], old_opID); // a ld_acq by the recover operation could be ocncurrent with the write to rcas_ann[tID][old_tID]
		MANUAL(FLUSH(&rcas_ann[tID][old_tID]));
	}
	RCas<PT> new_v;
	rcas_init(new_v, new_ptr, tID, opID);
	return utils::CAS(loc, x, new_v);	
}

template<class PT>
std::pair<uint32_t, bool> recover(RCas<PT> volatile * loc, uint32_t tID) {
	uint64_t ids = loc->id;
	RFLUSH(BARRIER(loc));
	MANUAL(BARRIER(loc));
	if(TID(ids) == tID) return std::make_pair(OPID(ids), 1);
	uint32_t max = 0;		
	for(int i = 0; i < MAX_THREADS; i++)
	{
		uint32_t tmp = utils::READ(rcas_ann[i][tID]);
		MANUAL(FLUSH(&rcas_ann[i][tID]));
		if(tmp > max)
			max = tmp;
	}
	MANUAL(MFENCE());
	if(max == 0) return std::make_pair(0, 0);
	return std::make_pair(max, 1);	
}


#endif /* P_UTILS_H_ */
