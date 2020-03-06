/*
 * Queue.h
 *
 *  Created on: Aug 2, 2018
 *      Author: https://arxiv.org/pdf/1806.04780.pdf
 *
 * Minor modifications by Pedro Ramalhete pramalhe@gmail.ocm
 */

#ifndef NORMAL_QUEUE_OPT_H_
#define NORMAL_QUEUE_OPT_H_

#include "Utilities.h"
#include "p_utils.h"

// Size of the persistent memory region
#ifndef PM_REGION_SIZE
#define PM_REGION_SIZE (2*1024*1024*1024ULL) // 2GB by default (to run on laptop)
#endif
// Name of persistent file mapping
#ifndef PM_FILE_NAME
#define PM_FILE_NAME   "/dev/shm/pmdk_shared"
#endif


#ifdef USE_PMDK_ALLOC
#include <libpmemobj++/p.hpp>
#include <libpmemobj++/transaction.hpp>
#include <libpmemobj++/pool.hpp>
#include <libpmemobj++/allocator.hpp>
#include <mutex>
using namespace pmem::obj;
auto gpopf = pool_base::create(PM_FILE_NAME, "", (size_t)(PM_REGION_SIZE));
std::mutex glockf {};
#endif

// If you see an error like this:
// terminate called after throwing an instance of 'pmem::transaction_error'
//  what():  transaction aborted
// It's because the pmdk pool is out of memory (this queue doesn't do deletion)


template <class T> class NormalQueueOpt{

	public:

    template<typename TN> static void internalDelete(TN* obj) {
#ifdef USE_PMDK_ALLOC
        if (obj == nullptr) return;
        obj->~TN();
        glockf.lock();
        transaction::run(gpopf, [obj] () {
            pmemobj_tx_free(pmemobj_oid(obj));
        });
        glockf.unlock();
#else
        delete obj;
#endif
    }

    template<typename TN, typename... Args> static TN* internalNew(Args&&... args) {
#ifdef USE_PMDK_ALLOC
        glockf.lock();
#ifdef MEASURE_PWB
        tl_num_pwbs += 3;
        tl_num_pfences += 2;
#endif
        void *addr = nullptr;
        transaction::run(gpopf, [&addr] () {
            auto oid = pmemobj_tx_alloc(sizeof(TN), 0);
            addr = pmemobj_direct(oid);
        });
        glockf.unlock();
        return new (addr) TN(std::forward<Args>(args)...); // placement new
#else
        return new TN(std::forward<Args>(args)...);
#endif
    }

    T EMPTY {};

    static std::string className() { return "Normalized-Opt Queue"; }

		//====================Start Node Class==========================//
		class Node {
			public:
				RCas<Node*> volatile next;
				T value;
                Node(T val) : value(val) { rcas_init(next); }
                Node() : value() { rcas_init(next); }
		};
		//====================End Node Class==========================//

		NormalQueueOpt() {
		    Node* dummy = internalNew<Node>(INT_MAX);
		    BARRIER(dummy);
            rcas_init(head, dummy);
            tail = dummy;
            BARRIER(&head);
            BARRIER(&tail);
		}

		//---------------------------------
		void enqueue(T value, int32_t threadID) {
		    //printf("enqueue\n");
			Node* node = internalNew<Node>(value);
			FLUSH(node);   // There will be a fence before the next boundary
			RCas<Node*> volatile *cas_loc;
			while (true) {
				//begin setup
				Node* last = utils::READ(tail);
				Node* next = rcas_read(last->next);
				cas_loc = NULL;
				if (last == utils::READ(tail)) {
					if (next == NULL) {
						cas_loc = &last->next;
					} else {
						MANUAL(FLUSH(&last->next));
						utils::CAS(&tail, last, next);
					}
				}
				
				//begin cas-executor
				if (cas_loc)
				{	
					MANUAL(FLUSH(&tail)); 
					MANUAL(FENCE()); // because the next boundary does not contain a fence
					capsule_boundary_opt(threadID, (void*) node, (void*) cas_loc); //The flush here can be reordered with the flush of tail and it is safe.
					if(rcas_cas(cas_loc, (Node*) NULL, node, threadID, get_capsule_number(threadID)))
					{
						//wrap up
						MANUAL(FLUSH(cas_loc));
						utils::CAS(&tail, (Node*)cas_loc, node); //last = cas_loc
						return;
					}
				}

				//other branch of wrap up
			}
		}

		//---------------------------------
		T dequeue(int threadID){
			while (true) {
				Node* first = rcas_read(head);
				Node* last = utils::READ(tail);
				Node* next = rcas_read(first->next);

				bool do_cas = false;

				if (first == rcas_read(head)) {
					if (first == last) {
						if (next == NULL) {
							MANUAL(BARRIER(&head));
							return INT_MIN;
						}
						MANUAL(FLUSH(&last->next));
						utils::CAS(&tail, last, next);
						MANUAL(BARRIER(&tail)); // Need to flush tail before next capsule boundary
					} else {
						do_cas = true;
					}
				}

				if(do_cas)
				{
					capsule_boundary_opt(threadID, (void*) next, first);
					if (rcas_cas(&head, first, next, threadID, get_capsule_number(threadID))){
						//wrap up
						MANUAL(BARRIER(&head));
						return next->value; // read of immutable value
					}					
				}

				// other branch of wrap up
			}
		}

	private:
		RCas<Node*> volatile head;
		int padding[PADDING];
		Node* volatile tail;
		int padding2[PADDING];
};

#endif /* LOCK_FREE_QUEUE_GENERAL_H_ */
