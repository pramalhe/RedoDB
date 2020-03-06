CXX = g++-8
# for running on /dev/shm (not on optane persistent memory)
CXXFLAGS = -std=c++14 -g -O3 -DPWB_IS_CLFLUSHOPT -DPM_REGION_SIZE=2*1024*1024*1024ULL
# for castor-1
#CXXFLAGS = -std=c++14 -g -O3 -DPWB_IS_CLWB -DPM_REGION_SIZE=64*1024*1024*1024ULL -DPM_USE_DAX -DPM_FILE_NAME="\"/mnt/pmem0/durable\""

# To have latencies at the end, add -DMEASURE_FUNC_TIMES

# For debugging, use ASAN
#CXXFLAGS += -fuse-ld=gold  -fsanitize=address

# Possible options for PWB are:
# -DPWB_IS_CLFLUSH		pwb is a CLFLUSH and pfence/psync are nops
# -DPWB_IS_CLFLUSHOPT	pwb is a CLFLUSHOPT and pfence/psync are SFENCE 
# -DPWB_IS_CLWB			pwb is a CLWB and pfence/psync are SFENCE
# -DPWB_IS_NOP			pwb/pfence/psync are nops. Used for shared memory persistence

INCLUDES = -I../

# This library is needed for PMDK
PMDKLIBS=-I../../libpmemobj-cpp/include/ -L/usr/local/lib -lpmemobj


# We make different binaries, one for every different PTM
BINARIES = \
	bin/pq-ll-enq-deq-ofwf \
	bin/pq-ll-enq-deq-cxptm \
	bin/pq-ll-enq-deq-redo \
    bin/pq-ll-enq-deq-redotimed \
    bin/pq-ll-enq-deq-redoopt \
	bin/psps-integer-ofwf \
	bin/pset-hash-1k-ofwf \
	bin/pset-hash-10k-ofwf \
	bin/pset-hash-1m-ofwf \
	bin/pset-ll-1k-ofwf \
	bin/pset-ll-10k-ofwf \
	bin/pset-tree-1k-ofwf \
	bin/pset-tree-10k-ofwf \
	bin/pset-tree-1m-ofwf \
	bin/pset-skiplist-1m-ofwf \
	bin/pset-tree-1m-dedicated-ofwf \
	bin/psps-integer-cxptm \
	bin/pset-hash-1k-cxptm \
	bin/pset-hash-10k-cxptm \
	bin/pset-hash-1m-cxptm \
	bin/pset-ll-1k-cxptm \
	bin/pset-ll-10k-cxptm \
	bin/pset-tree-1k-cxptm \
	bin/pset-tree-10k-cxptm \
	bin/pset-tree-1m-cxptm \
	bin/pset-skiplist-1m-cxptm \
	bin/pset-tree-1m-dedicated-cxptm \
	bin/psps-integer-redo \
	bin/pset-hash-1k-redo \
	bin/pset-hash-10k-redo \
	bin/pset-hash-1m-redo \
	bin/pset-ll-1k-redo \
	bin/pset-ll-10k-redo \
	bin/pset-tree-1k-redo \
	bin/pset-tree-10k-redo \
	bin/pset-tree-1m-redo \
	bin/pset-skiplist-1m-redo \
	bin/pset-tree-1m-dedicated-redo \
	bin/psps-integer-redotimed \
	bin/pset-hash-1k-redotimed \
	bin/pset-hash-10k-redotimed \
	bin/pset-hash-1m-redotimed \
	bin/pset-ll-1k-redotimed \
	bin/pset-ll-10k-redotimed \
	bin/pset-tree-1k-redotimed \
	bin/pset-tree-10k-redotimed \
	bin/pset-tree-1m-redotimed \
	bin/pset-skiplist-1m-redotimed \
	bin/pset-tree-1m-dedicated-redotimed \
	bin/pset-ll-1k-cxpuc \
	bin/pset-hash-1k-cxpuc \
	bin/pset-hash-10k-cxpuc \
	bin/pset-hash-1m-cxpuc \
	bin/pset-ll-10k-cxpuc \
	bin/pset-tree-1k-cxpuc \
	bin/pset-tree-10k-cxpuc \
	bin/pset-tree-1m-cxpuc \
	bin/pq-ll-enq-deq-cxpuc \
	bin/pset-tree-1m-dedicated-cxpuc \
	bin/psps-integer-redoopt \
	bin/pset-hash-10k-redoopt \
	bin/pset-hash-1m-redoopt \
	bin/pset-tree-10k-redoopt \
	bin/pset-tree-1m-redoopt \
	bin/pset-hash-1k-redoopt \
	bin/pset-tree-1k-redoopt \
	bin/pset-ll-10k-redoopt \
	bin/pset-tree-1m-dedicated-redoopt \
	bin/pq-ll-enq-deq-friedman-volatile \
	bin/pq-ll-enq-deq-normopt-volatile \
	bin/pq-ll-enq-deq-redoopt-volatile \
	bin/pq-latency-ofwf \
	bin/pq-latency-cxpuc \
	bin/pq-latency-cxptm \
	bin/pq-latency-redo \
	bin/pq-latency-redotimed \
	bin/pq-latency-redoopt \
	bin/pq-latency-pmdk \
	bin/pq-latency-friedman \
	bin/pq-latency-normopt \
#	bin/pq-pwb-romlr \
	bin/pq-pwb-ofwf \
	bin/pq-pwb-cxptm \
	bin/pq-pwb-redo \
    bin/pq-pwb-redotimed \
    bin/pq-pwb-redoopt \
	bin/pq-pwb-friedman \
	bin/pq-pwb-normopt \
	bin/pq-pwb-pmdk \
#	bin/pset-ll-1k-redoopt \
	bin/pset-skiplist-1m-redoopt \
#	bin/hydra-redo \
	bin/hydra-romlog \
	bin/hydra-ofwf \
	bin/hydra-redologtl2 \
	bin/hydra-sps-redo \
	bin/hydra-sps-romlog \
	bin/hydra-sps-ofwf \
	bin/hydra-sps-redologtl2 \
#	bin/hydra-hash-ofwf \
	bin/hydra-hash-redo \
	bin/hydra-hash-redologtl2 \
#	bin/psps-integer-cxpuc \
	
	

PTMS = \
	../ptms/pmdk/PMDKTM.hpp \
	../ptms/ponefilelf/OneFilePTMLF.hpp \
	../ptms/ponefilewf/OneFilePTMWF.hpp \
	../ptms/cxptm/CircularArray.hpp \
	../ptms/cxptm/CXPTM.hpp \
	../ptms/cxptm/CXPTM.cpp \
	../ptms/cxptm/malloc.cpp \
	../ptms/cxpuc/CXPUC.hpp \
	../ptms/cxpuc/CXPUC.cpp \
	../ptms/cxpuc/malloc.cpp \
	../ptms/redo/Redo.hpp \
	../ptms/redo/Redo.cpp \
	../ptms/redo/malloc.cpp \
	../ptms/redotimed/RedoTimed.hpp \
	../ptms/redotimed/RedoTimed.cpp \
	../ptms/redotimed/malloc.cpp \
	../common/pfences.h \
	lib/libromulus.a \
	
	
CXPTM_SRCS = \
	../common/ThreadRegistry.cpp \
	../ptms/cxptm/CXPTM.cpp \
#	../ptms/cxptm/malloc.cpp \

CXPUC_SRCS = \
	../common/ThreadRegistry.cpp \
	../ptms/cxpuc/CXPUC.cpp \
#	../ptms/cxpuc/malloc.cpp \

REDO_SRCS = \
	../common/ThreadRegistry.cpp \
	../ptms/redo/Redo.cpp \
	
REDOTIMED_SRCS = \
	../common/ThreadRegistry.cpp \
	../ptms/redotimed/RedoTimed.cpp \

REDOOPT_SRCS = \
	../common/ThreadRegistry.cpp \
	../ptms/redoopt/RedoOpt.cpp \

#
# Dependencies for each PTM
#
COMMON_DEP = \
	../common/ThreadRegistry.hpp \
	../common/EsLoco.hpp \
	../common/StrongTryRIRWLock.hpp \
		
CXPTM_DEP = \
	$(CXPTM_SRCS) \
	$(COMMON_DEP) \
	../ptms/cxptm/CXPTM.hpp \

CXPUC_DEP = \
	$(CXPUC_SRCS) \
	$(COMMON_DEP) \
	../ptms/cxpuc/CXPUC.hpp \

REDO_DEP = \
	$(REDO_SRCS) \
	$(COMMON_DEP) \
	../ptms/redo/Redo.hpp \

REDOTIMED_DEP = \
	$(REDOTIMED_SRCS) \
	$(COMMON_DEP) \
	../ptms/redotimed/RedoTimed.hpp \
	
REDOTIMEDHASH_DEP = \
	$(REDOTIMEDHASH_SRCS) \
	$(COMMON_DEP) \
	../ptms/redotimedhash/RedoTimedHash.hpp \
	

PQUEUES_DEP = \
	../pdatastructures/pqueues/PFriedmanQueue.hpp \
	../pdatastructures/pqueues/NormalQueue-opt.hpp \
	../pdatastructures/pqueues/PMDKLinkedListQueue.hpp \
	../pdatastructures/pqueues/TMLinkedListQueue.hpp \
	../pdatastructures/pqueues/POFWFLinkedListQueue.hpp \
	../pdatastructures/pqueues/RedoOptLinkedListQueue.hpp \


all: $(BINARIES) persistencyclean

clean: persistencyclean
	rm -f bin/*
	rm -f lib/*

persistencyclean:
	rm -f /mnt/pmem0/durable
	rm -f /mnt/pmem1/durable
	rm -f /dev/shm/*_shared
	rm -f /dev/shm/psegments/*
	rm -f /dev/shm/pmdk_shared

# Do "make pmdk" to build the binaries for pmdk (libpmemobj) benchmarks. 
# Needs pmdk installed on your system. 
pmdk: \
	bin/psps-integer-pmdk \
	bin/pset-hash-1k-pmdk \
	bin/pset-hash-10k-pmdk \
	bin/pset-hash-1m-pmdk \
	bin/pset-ll-1k-pmdk \
	bin/pset-ll-10k-pmdk \
	bin/pset-tree-1k-pmdk \
	bin/pset-tree-10k-pmdk \
	bin/pset-tree-1m-pmdk \
	bin/pset-tree-1m-dedicated-pmdk \
	bin/pset-skiplist-1m-pmdk \
	bin/pq-ll-enq-deq-pmdk \
	bin/pq-ll-enq-deq-friedman \
	bin/pq-ll-enq-deq-normopt \
	persistencyclean \

# This is to build executables that measure the number of pwbs (for the queues). For PMDK it needs a patch.
queues-pwbs: \
    bin/pq-pwb-redoopt \
	bin/pq-pwb-friedman \
	bin/pq-pwb-normopt \
	persistencyclean \
#	bin/pq-pwb-pmdk \
	bin/pq-pwb-ofwf \
	bin/pq-pwb-cxptm \
	bin/pq-pwb-redo \
    bin/pq-pwb-redotimed \




#
# Queues for persistent memory
#

bin/pq-ll-enq-deq-ofwf: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-ofwf -lpthread

bin/pq-ll-enq-deq-cxpuc: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(CXPUC_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-cxpuc -lpthread

bin/pq-ll-enq-deq-cxptm: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(CXPTM_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-cxptm -lpthread

bin/pq-ll-enq-deq-redo: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDO_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-redo -lpthread
	
bin/pq-ll-enq-deq-redotimed: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDOTIMED_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-redotimed -lpthread

bin/pq-ll-enq-deq-redoopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDOOPT_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-redoopt -lpthread

bin/pq-ll-enq-deq-pmdk: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) $(CX_SRCS) $(CSRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-pmdk -lpthread $(PMDKLIBS)

bin/pq-ll-enq-deq-friedman: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_FRIEDMAN -DUSE_PMDK_ALLOC $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-friedman -lpthread $(PMDKLIBS)

bin/pq-ll-enq-deq-normopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_NORMALIZED_OPT -DUSE_PMDK_ALLOC -mcx16 -DMCX16 -DMANUAL_FLUSH $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-normopt -lpthread $(PMDKLIBS)

#
# volatile queues
#
bin/pq-ll-enq-deq-friedman-volatile: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_FRIEDMAN_VOLATILE $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-friedman-volatile -lpthread

bin/pq-ll-enq-deq-normopt-volatile: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_NORMALIZED_OPT_VOLATILE -mcx16 -DMCX16 -DMANUAL_FLUSH $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-normopt-volatile -lpthread
	
bin/pq-ll-enq-deq-redoopt-volatile: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDOOPT_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT_VOLATILE $(INCLUDES) $(REDOOPT_SRCS) pq-ll-enq-deq.cpp -o bin/pq-ll-enq-deq-redoopt-volatile -lpthread



#
# Latency (queues) for persistent memory
#

bin/pq-latency-ofwf: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pq-latency.cpp -o bin/pq-latency-ofwf -lpthread

bin/pq-latency-cxpuc: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp $(CXPUC_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pq-latency.cpp -o bin/pq-latency-cxpuc -lpthread

bin/pq-latency-cxptm: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp $(CXPTM_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pq-latency.cpp -o bin/pq-latency-cxptm -lpthread

bin/pq-latency-redo: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp $(REDO_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pq-latency.cpp -o bin/pq-latency-redo -lpthread
	
bin/pq-latency-redotimed: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp $(REDOTIMED_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pq-latency.cpp -o bin/pq-latency-redotimed -lpthread

bin/pq-latency-redoopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp $(REDOOPT_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pq-latency.cpp -o bin/pq-latency-redoopt -lpthread

bin/pq-latency-pmdk: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) $(CX_SRCS) $(CSRCS) pq-latency.cpp -o bin/pq-latency-pmdk -lpthread $(PMDKLIBS)

bin/pq-latency-friedman: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_FRIEDMAN -DUSE_PMDK_ALLOC $(INCLUDES) pq-latency.cpp -o bin/pq-latency-friedman -lpthread $(PMDKLIBS)

bin/pq-latency-normopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkLatencyQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_NORMALIZED_OPT -DUSE_PMDK_ALLOC -mcx16 -DMCX16 -DMANUAL_FLUSH $(INCLUDES) pq-latency.cpp -o bin/pq-latency-normopt -lpthread $(PMDKLIBS)


#
# Measure PWBs for queues on persistent memory
#

bin/pq-pwb-ofwf: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF -DMEASURE_PWB $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-pwb-ofwf -lpthread

bin/pq-pwb-cxpuc: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(CXPUC_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC -DMEASURE_PWB $(INCLUDES) $(CXPUC_SRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-cxpuc -lpthread

bin/pq-pwb-cxptm: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(CXPTM_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM -DMEASURE_PWB $(INCLUDES) $(CXPTM_SRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-cxptm -lpthread

bin/pq-pwb-redo: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDO_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDO -DMEASURE_PWB $(INCLUDES) $(REDO_SRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-redo -lpthread
	
bin/pq-pwb-redotimed: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDOTIMED_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED -DMEASURE_PWB $(INCLUDES) $(REDOTIMED_SRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-redotimed -lpthread

bin/pq-pwb-redoopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp $(REDOOPT_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT -DMEASURE_PWB $(INCLUDES) $(REDOOPT_SRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-redoopt -lpthread

bin/pq-pwb-pmdk: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK -DMEASURE_PWB $(INCLUDES) $(CX_SRCS) $(CSRCS) pq-ll-enq-deq.cpp -o bin/pq-pwb-pmdk -lpthread $(PMDKLIBS)

bin/pq-pwb-friedman: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_FRIEDMAN -DUSE_PMDK_ALLOC -DMEASURE_PWB $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-pwb-friedman -lpthread $(PMDKLIBS) -lpmem

bin/pq-pwb-normopt: pq-ll-enq-deq.cpp $(PQUEUES_DEP) PBenchmarkQueues.hpp
	$(CXX) $(CXXFLAGS) -DUSE_NORMALIZED_OPT -DUSE_PMDK_ALLOC -DMEASURE_PWB -mcx16 -DMCX16 -DMANUAL_FLUSH $(INCLUDES) pq-ll-enq-deq.cpp -o bin/pq-pwb-normopt -lpthread $(PMDKLIBS)


	
#
# Linked List Sets for persistent memory
#

bin/pset-ll-1k-ofwf: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-ll-1k.cpp -o bin/pset-ll-1k-ofwf -lpthread

bin/pset-ll-1k-cxpuc: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(CXPUC_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-ll-1k.cpp -o bin/pset-ll-1k-cxpuc -lpthread

bin/pset-ll-1k-cxptm: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(CXPTM_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-ll-1k.cpp -o bin/pset-ll-1k-cxptm -lpthread

bin/pset-ll-1k-redo: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(CXREDO_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-ll-1k.cpp -o bin/pset-ll-1k-redo -lpthread
	
bin/pset-ll-1k-redotimed: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(REDOTIMED_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-ll-1k.cpp -o bin/pset-ll-1k-redotimed -lpthread

bin/pset-ll-1k-pmdk: pset-ll-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-ll-1k.cpp -o bin/pset-ll-1k-pmdk -lpthread $(PMDKLIBS)


bin/pset-ll-10k-romlr: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp lib/libromulus.a 
	$(CXX) $(CXXFLAGS) -DUSE_ROMLR $(INCLUDES) pset-ll-10k.cpp -o bin/pset-ll-10k-romlr -lpthread lib/libromulus.a

bin/pset-ll-10k-ofwf: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-ll-10k.cpp -o bin/pset-ll-10k-ofwf -lpthread

bin/pset-ll-10k-cxpuc: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(CXPUC_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-ll-10k.cpp -o bin/pset-ll-10k-cxpuc -lpthread

bin/pset-ll-10k-cxptm: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(CXPTM_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-ll-10k.cpp -o bin/pset-ll-10k-cxptm -lpthread

bin/pset-ll-10k-redo: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(REDO_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-ll-10k.cpp -o bin/pset-ll-10k-redo -lpthread

bin/pset-ll-10k-redotimed: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(REDOTIMED_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-ll-10k.cpp -o bin/pset-ll-10k-redotimed -lpthread

bin/pset-ll-10k-redoopt: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp $(REDOOPT_DEP) 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-ll-10k.cpp -o bin/pset-ll-10k-redoopt -lpthread
	
bin/pset-ll-10k-pmdk: pset-ll-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMLinkedListSet.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-ll-10k.cpp -o bin/pset-ll-10k-pmdk -lpthread $(PMDKLIBS)



#
# Hash Sets for persistent memory
#

bin/pset-hash-1k-ofwf: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-hash-1k.cpp -o bin/pset-hash-1k-ofwf -lpthread

bin/pset-hash-1k-cxpuc: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-hash-1k.cpp -o bin/pset-hash-1k-cxpuc -lpthread

bin/pset-hash-1k-cxptm: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-hash-1k.cpp -o bin/pset-hash-1k-cxptm -lpthread

bin/pset-hash-1k-redo: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-hash-1k.cpp -o bin/pset-hash-1k-redo -lpthread

bin/pset-hash-1k-redotimed: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-hash-1k.cpp -o bin/pset-hash-1k-redotimed -lpthread
	
bin/pset-hash-1k-redoopt: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-hash-1k.cpp -o bin/pset-hash-1k-redoopt -lpthread
	
bin/pset-hash-1k-pmdk: pset-hash-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-hash-1k.cpp -o bin/pset-hash-1k-pmdk -lpthread $(PMDKLIBS)



bin/pset-hash-10k-ofwf: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-hash-10k.cpp -o bin/pset-hash-10k-ofwf -lpthread

bin/pset-hash-10k-cxpuc: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-hash-10k.cpp -o bin/pset-hash-10k-cxpuc -lpthread

bin/pset-hash-10k-cxptm: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-hash-10k.cpp -o bin/pset-hash-10k-cxptm -lpthread

bin/pset-hash-10k-redo: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-hash-10k.cpp -o bin/pset-hash-10k-redo -lpthread

bin/pset-hash-10k-redotimed: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-hash-10k.cpp -o bin/pset-hash-10k-redotimed -lpthread
	
bin/pset-hash-10k-redoopt: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-hash-10k.cpp -o bin/pset-hash-10k-redoopt -lpthread
	
bin/pset-hash-10k-pmdk: pset-hash-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-hash-10k.cpp -o bin/pset-hash-10k-pmdk -lpthread $(PMDKLIBS)



bin/pset-hash-1m-ofwf: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-hash-1m.cpp -o bin/pset-hash-1m-ofwf -lpthread

bin/pset-hash-1m-cxpuc: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-hash-1m.cpp -o bin/pset-hash-1m-cxpuc -lpthread

bin/pset-hash-1m-cxptm: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-hash-1m.cpp -o bin/pset-hash-1m-cxptm -lpthread

bin/pset-hash-1m-redo: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-hash-1m.cpp -o bin/pset-hash-1m-redo -lpthread

bin/pset-hash-1m-redotimed: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-hash-1m.cpp -o bin/pset-hash-1m-redotimed -lpthread

bin/pset-hash-1m-redoopt: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-hash-1m.cpp -o bin/pset-hash-1m-redoopt -lpthread

bin/pset-hash-1m-pmdk: pset-hash-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMap.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-hash-1m.cpp -o bin/pset-hash-1m-pmdk -lpthread $(PMDKLIBS)


#
# Trees for persistent memory
#

bin/pset-tree-1k-ofwf: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-tree-1k.cpp -o bin/pset-tree-1k-ofwf -lpthread

bin/pset-tree-1k-cxpuc: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-tree-1k.cpp -o bin/pset-tree-1k-cxpuc -lpthread

bin/pset-tree-1k-cxptm: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-tree-1k.cpp -o bin/pset-tree-1k-cxptm -lpthread

bin/pset-tree-1k-redo: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-tree-1k.cpp -o bin/pset-tree-1k-redo -lpthread

bin/pset-tree-1k-redotimed: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-tree-1k.cpp -o bin/pset-tree-1k-redotimed -lpthread

bin/pset-tree-1k-redoopt: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-tree-1k.cpp -o bin/pset-tree-1k-redoopt -lpthread
	
bin/pset-tree-1k-pmdk: pset-tree-1k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/pmdk/PMDKTM.hpp 
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-tree-1k.cpp -o bin/pset-tree-1k-pmdk -lpthread $(PMDKLIBS)
	


bin/pset-tree-10k-ofwf: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-tree-10k.cpp -o bin/pset-tree-10k-ofwf -lpthread

bin/pset-tree-10k-cxpuc: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-tree-10k.cpp -o bin/pset-tree-10k-cxpuc -lpthread

bin/pset-tree-10k-cxptm: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-tree-10k.cpp -o bin/pset-tree-10k-cxptm -lpthread

bin/pset-tree-10k-redo: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-tree-10k.cpp -o bin/pset-tree-10k-redo -lpthread

bin/pset-tree-10k-redotimed: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-tree-10k.cpp -o bin/pset-tree-10k-redotimed -lpthread

bin/pset-tree-10k-redoopt: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-tree-10k.cpp -o bin/pset-tree-10k-redoopt -lpthread

bin/pset-tree-10k-pmdk: pset-tree-10k.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/pmdk/PMDKTM.hpp 
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-tree-10k.cpp -o bin/pset-tree-10k-pmdk -lpthread $(PMDKLIBS)
	
	


bin/pset-tree-1m-ofwf: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-tree-1m.cpp -o bin/pset-tree-1m-ofwf -lpthread

bin/pset-tree-1m-cxpuc: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-tree-1m.cpp -o bin/pset-tree-1m-cxpuc -lpthread

bin/pset-tree-1m-cxptm: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-tree-1m.cpp -o bin/pset-tree-1m-cxptm -lpthread

bin/pset-tree-1m-redo: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-tree-1m.cpp -o bin/pset-tree-1m-redo -lpthread

bin/pset-tree-1m-redotimed: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-tree-1m.cpp -o bin/pset-tree-1m-redotimed -lpthread

bin/pset-tree-1m-redoopt: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-tree-1m.cpp -o bin/pset-tree-1m-redoopt -lpthread

bin/pset-tree-1m-pmdk: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/pmdk/PMDKTM.hpp 
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-tree-1m.cpp -o bin/pset-tree-1m-pmdk -lpthread $(PMDKLIBS)


bin/pset-tree-1m-dedicated-ofwf: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-ofwf -lpthread

bin/pset-tree-1m-dedicated-cxpuc: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-cxpuc -lpthread

bin/pset-tree-1m-dedicated-cxptm: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-cxptm -lpthread

bin/pset-tree-1m-dedicated-redo: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-redo -lpthread

bin/pset-tree-1m-dedicated-redotimed: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-redotimed -lpthread
	
bin/pset-tree-1m-dedicated-redoopt: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-redoopt -lpthread

bin/pset-tree-1m-dedicated-pmdk: pset-tree-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/pmdk/PMDKTM.hpp 
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-tree-1m-dedicated.cpp -o bin/pset-tree-1m-dedicated-pmdk -lpthread $(PMDKLIBS)


#
# SkipLists for persistent memory
#

bin/pset-skiplist-1m-ofwf: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipList.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-ofwf -lpthread

bin/pset-skiplist-1m-cxpuc: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipList.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-cxpuc -lpthread

bin/pset-skiplist-1m-cxptm: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipList.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-cxptm -lpthread

bin/pset-skiplist-1m-redo: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipList.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-redo -lpthread

bin/pset-skiplist-1m-redotimed: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipList.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-redotimed -lpthread

bin/pset-skiplist-1m-pmdk: pset-skiplist-1m.cpp PBenchmarkSets.hpp ../pdatastructures/TMSkipListByRef.hpp ../ptms/pmdk/PMDKTM.hpp 
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) pset-skiplist-1m.cpp -o bin/pset-skiplist-1m-pmdk -lpthread $(PMDKLIBS)


	

	
#
# SPS for persistent memory
#

bin/psps-integer-ofwf: psps-integer.cpp PBenchmarkSPS.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) psps-integer.cpp -o bin/psps-integer-ofwf -lpthread

bin/psps-integer-cxpuc: psps-integer.cpp PBenchmarkSPS.hpp $(CXPUC_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPUC $(INCLUDES) $(CXPUC_SRCS) psps-integer.cpp -o bin/psps-integer-cxpuc -lpthread

bin/psps-integer-cxptm: psps-integer.cpp PBenchmarkSPS.hpp $(CXPTM_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_CXPTM $(INCLUDES) $(CXPTM_SRCS) psps-integer.cpp -o bin/psps-integer-cxptm -lpthread

bin/psps-integer-redo: psps-integer.cpp PBenchmarkSPS.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) psps-integer.cpp -o bin/psps-integer-redo -lpthread

bin/psps-integer-redotimed: psps-integer.cpp PBenchmarkSPS.hpp $(REDOTIMED_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED $(INCLUDES) $(REDOTIMED_SRCS) psps-integer.cpp -o bin/psps-integer-redotimed -lpthread

bin/psps-integer-redoopt: psps-integer.cpp PBenchmarkSPS.hpp $(REDOOPT_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT $(INCLUDES) $(REDOOPT_SRCS) psps-integer.cpp -o bin/psps-integer-redoopt -lpthread

bin/psps-integer-pmdk: psps-integer.cpp PBenchmarkSPS.hpp ../ptms/pmdk/PMDKTM.hpp
	$(CXX) $(CXXFLAGS) -DUSE_PMDK $(INCLUDES) psps-integer.cpp -o bin/psps-integer-pmdk -lpthread $(PMDKLIBS)

	
	
#
# Plots for Hydra (red-black tree with 100% and 0%)
#

bin/hydra-redo: hydra.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) hydra.cpp -o bin/hydra-redo -lpthread

bin/hydra-ofwf: hydra.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) hydra.cpp -o bin/hydra-ofwf -lpthread

bin/hydra-redologtl2: hydra.cpp PBenchmarkSets.hpp ../pdatastructures/TMRedBlackTree.hpp ../ptms/redolog/RedoLogTL2.hpp
	$(CXX) $(CXXFLAGS) -DUSE_TL2_REDO $(INCLUDES) hydra.cpp -o bin/hydra-redologtl2 -lpthread
	
#
# Plots for Hydra (fixed size hashmap with 100% and 0%)
#

bin/hydra-hash-redo: hydra-hash.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMapFixedSize.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) hydra-hash.cpp -o bin/hydra-hash-redo -lpthread

bin/hydra-hash-ofwf: hydra-hash.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMapFixedSize.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) hydra-hash.cpp -o bin/hydra-hash-ofwf -lpthread

bin/hydra-hash-redologtl2: hydra-hash.cpp PBenchmarkSets.hpp ../pdatastructures/TMHashMapFixedSize.hpp ../ptms/redolog/RedoLogTL2.hpp
	$(CXX) $(CXXFLAGS) -DUSE_TL2_REDO $(INCLUDES) hydra-hash.cpp -o bin/hydra-hash-redologtl2 -lpthread
	
#
# Plots for Hydra (SPS)
#

bin/hydra-sps-redo: hydra-sps.cpp PBenchmarkSets.hpp $(REDO_DEP)
	$(CXX) $(CXXFLAGS) -DUSE_REDO $(INCLUDES) $(REDO_SRCS) hydra-sps.cpp -o bin/hydra-sps-redo -lpthread

bin/hydra-sps-ofwf: hydra-sps.cpp PBenchmarkSets.hpp ../ptms/ponefilewf/OneFilePTMWF.hpp
	$(CXX) $(CXXFLAGS) -DUSE_OFWF $(INCLUDES) hydra-sps.cpp -o bin/hydra-sps-ofwf -lpthread

bin/hydra-sps-redologtl2: hydra-sps.cpp PBenchmarkSets.hpp ../ptms/redolog/RedoLogTL2.hpp
	$(CXX) $(CXXFLAGS) -DUSE_TL2_REDO $(INCLUDES) hydra-sps.cpp -o bin/hydra-sps-redologtl2 -lpthread
	
