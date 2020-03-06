We have in here the PTMs (Persistent Transactional Memories) and their wrappers.

### PMDK ###
Also known as NVML, it is undo-log based.
Uses a regular pthread_rwlock_t for concurrency.
Blocking progress.
You need to install this library from github.
Uses ...

### RomulusLog ###
For persistence, uses the Romulus technique with volatile redo log.
Uses a C-RW-WP reader-writer lock with integrated flat-combining.
Blocking with starvation-free progress for writers.
Uses 0x7fdd40000000 by default as the mapping address.

### RomulusLR ###
For persistence, uses the Romulus technique with volatile redo log.
For concurrency, uses the Left-Right universal construct with integrated flat-combining.
Blocking with starvation-free progress for writers and wait-free population oblivious for readers.
Uses 0x7fdd80000000 by default as the mapping address.

### OneFile PTM Lock-Free ###
Implementation of the OneFile STM (Lock-Free) with persistent memory support. Does 2 fences per transaction.
This is undo-log based and it uses EsLoco memory allocator.
Has lock-free progress for all transactions.
Does not use pfences.h
Uses 0x7fea00000000 by default as the mapping address.

### PM OneFile Wait-Free ###
Implementation of the OneFile STM (Wait-Free) with persistent memory support. Does 2 fences per transaction.
This is undo-log based and it uses EsLoco memory allocator.
Has wait-free bounded progress for all transactions.
Does not use pfences.h
Uses 0x7feb00000000 by default as the mapping address.


### Trinity 2 Fences ###
    ptms/trinity2f/TrinityFC.hpp
This PTM uses the Trinitity 2 Fences algorithm which has 


The pfences.h file contains the definitions of the PWB(), PFENCE() and PSYNC() macros, depending on the target cpu.