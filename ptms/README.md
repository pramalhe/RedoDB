We have in here the PTMs (Persistent Transactional Memories) and their wrappers.

### PMDK ###
Also known as NVML.
Durability is given by a persistent undo log.
Uses a regular pthread_rwlock_t for concurrency. Blocking progress.
You need to install this library from github.

### CX-PUC ###
A Persistent Universal Construction.
Memory usage can be up to 2 x MAX_THREADS instances.
Durability is given by multiple instances. Uses a volatile queue to keep track of the transactions.
Concurrency is given by CX UC.
Does no load or store interposing but objects (data structures) need to have a copy constructor.

### CX-PTM ###
A Persistent Transactional Memory based on CX.
Memory usage can be up to 2 x MAX_THREADS instances.
Durability is given by multiple instances. Uses a volatile queue to keep track of the transactions.
Concurrency is given by CX UC.

### Redo-PTM ###
A Persistent Transactional Memory.
Memory usage can be up to MAX_THREADS+1 instances.
Durability is given by multiple instances. Uses a volatile queue and volatile redo-log.
Concurrency is given by Combineds with strong-try-locks.

### RedoTimed-PTM ###
A Persistent Transactional Memory.
Memory usage can be up to MAX_THREADS+1 instances.
Durability is given by multiple instances. Uses a volatile queue and volatile redo-log.
Concurrency is given by Combineds with strong-try-locks.
Has a backoff mechanism.

### RedoOpt-PTM ###
A Persistent Transactional Memory.
Memory usage can be up to MAX_THREADS+1 instances.
Durability is given by multiple instances. Uses a volatile queue and volatile redo-log.
Concurrency is given by Combineds with strong-try-locks.
Has a backoff mechanism and multiple other optimizations.


The pfences.h file contains the definitions of the PWB(), PFENCE() and PSYNC() macros, depending on the target cpu.