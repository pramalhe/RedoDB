CXX=g++-8
BIN=bin
CXXFLAGS= -std=c++14 -g -O2 -DPWB_IS_CLFLUSHOPT -DPM_REGION_SIZE=4*1024*1024*1024ULL
# for castor-1
#CXXFLAGS = -std=c++14 -g -O2 -DPWB_IS_CLWB -DPM_REGION_SIZE=122*1024*1024*1024ULL -DPM_USE_DAX -DPM_FILE_NAME="\"/mnt/pmem0/durable\""
# Possible options for PWB are:
# -DPWB_IS_CLFLUSH		pwb is a CLFLUSH and pfence/psync are nops      (Broadwell)
# -DPWB_IS_CLFLUSHOPT	pwb is a CLFLUSHOPT and pfence/psync are SFENCE (Kaby Lake) 
# -DPWB_IS_CLWB			pwb is a CLWB and pfence/psync are SFENCE       (Sky Lake SP, or Canon Lake SP and beyond)
# -DPWB_IS_NOP			pwb/pfence/psync are nops. Used for shared memory persistence

all: \
	persistencyclean \
	bin/ut2 \
	bin/db_bench_redo \
	bin/db_bench_redotimed \
	bin/db_bench_redoopt \

MYDEPS = \
	common/pfences.h \
	common/ThreadRegistry.hpp \
	common/ThreadRegistry.cpp \
	ptms/rwlocks/CRWWPSpinLock.hpp \
	ptms/redo/Redo.hpp \
	ptms/redo/Redo.cpp \
	ptms/redotimed/RedoTimed.hpp \
	ptms/redotimed/RedoTimed.cpp \
	ptms/redoopt/RedoOpt.hpp \
	ptms/redoopt/RedoOpt.cpp \
	db.h \
	iterator.h \
	options.h \
	slice.h \
	status.h \
	TMHashMap.hpp \
	TMHashMapIterator.h \
	db_impl.cc \
	db_impl.h \
	status.cc \
	db_bench.cc \
	ptmdb.h \
	util/random.h \
	util/histogram.cc \
	util/histogram.h \
	util/mutexlock.h \
	util/testutil.cc \
	port/port_posix.cc \
	write_batch.cc \
	write_batch.h \
	

SRCS = \
	db_impl.cc \
	status.cc \
	port/port_posix.cc \
	util/histogram.cc \
	util/env_posix.cc \
	util/testutil.cc \
	write_batch.cc \
	

REDO_SRCS = \
	common/ThreadRegistry.cpp \
	ptms/redo/Redo.cpp \
#	ptms/redo/malloc.cpp \

REDOTIMED_SRCS = \
	common/ThreadRegistry.cpp \
	ptms/redotimed/RedoTimed.cpp \
#	ptms/redotimed/malloc.cpp \

REDOOPT_SRCS = \
	common/ThreadRegistry.cpp \
	ptms/redoopt/RedoOpt.cpp \
	

# unit test 1

bin/ut2: $(MYDEPS) $(DSDEPS) examples/ut2.cpp
#	$(CXX) $(CXXFLAGS) -DUSE_ROMULUS_LOG ut2.cpp $(SRCS) $(ROMLOG_SRCS) -I. -Iutil -Iport -o bin/ut2 -lpthread
	$(CXX) $(CXXFLAGS) -DUSE_REDO examples/ut2.cpp $(SRCS) $(REDO_SRCS) -I. -Iutil -Iport -o bin/ut2 -lpthread

bin/recover_rocksdb: examples/recover_rocksdb.cpp
	$(CXX) $(CXXFLAGS) examples/recover_rocksdb.cpp -I~/rocksdb/include -o bin/recover_rocksdb -L~/rocksdb/ -lrocksdb -lpthread -lrt -lsnappy -lgflags -lz -lbz2 -llz4 -lzstd -lnuma -ltbb

bin/recover_redodb: examples/recover_redodb.cpp
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT examples/recover_redodb.cpp $(SRCS) $(REDOOPT_SRCS) -I. -Iutil -Iport -o bin/db_bench_redoopt -lpthread -o bin/recover_redodb


bin/db_bench_redo: $(MYDEPS) $(DSDEPS) db_bench.cc 
	$(CXX) $(CXXFLAGS) -DUSE_REDO db_bench.cc $(SRCS) $(REDO_SRCS) -I. -Iutil -Iport -o bin/db_bench_redo -lpthread

bin/db_bench_redotimed: $(MYDEPS) $(DSDEPS) db_bench.cc 
	$(CXX) $(CXXFLAGS) -DUSE_REDOTIMED db_bench.cc $(SRCS) $(REDOTIMED_SRCS) -I. -Iutil -Iport -o bin/db_bench_redotimed -lpthread

bin/db_bench_redoopt: $(MYDEPS) $(DSDEPS) db_bench.cc 
	$(CXX) $(CXXFLAGS) -DUSE_REDOOPT db_bench.cc $(SRCS) $(REDOOPT_SRCS) -I. -Iutil -Iport -o bin/db_bench_redoopt -lpthread

persistencyclean:
	rm -f /dev/shm/*_shared
	rm -f /mnt/pmem0/durable
	rm -rf /mnt/pmem0/rocksdb/


# We also clean the persistency files so that on a new build we get a clean slate
clean: persistencyclean
	rm -f bin/*
		
