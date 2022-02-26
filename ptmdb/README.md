# Redo DB #
A persistent in-memory key-value store for usage with NVMM
RedoDB uses the same API as RocksDB.
At the core of RedoDB is an Hash Map data structure protected with the RedoOpt PTM persistent transactional memory (PTM).


## How to run ##
To execute the LevelDB benchmarks for RedoDB, edit the Makefile to set the size of the pool in persistent memory and location of the DAX file system, then do this:
... 
    make
    ./db_bench_redoopt
or
    ./db_bench --benchmarks=fillbatch,fillsync
    ./db_bench --threads=4
    ./db_bench --reads=10000000
...

