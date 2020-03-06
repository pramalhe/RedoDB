
# RedoDB, a wait-free KV store for Persistent Memory

RedoOpt PTM is meant to make it easy to implement wait-free data structures in persistent memory (sometimes named non-volatile main memory).
It is based on the paper "[Persistent Memory and the Rise of Universal Constructions](https://github.com/pramalhe/RedoDB/blob/master/RedoDB-2020.pdf)" by Correia, Felber and Ramalhete
https://github.com/pramalhe/RedoDB/blob/master/RedoDB-2020.pdf

We used RedoOpt PTM to implement a KV store, RedoDB, that has an API similar to LevelDB/RocksDB.

Characteristics of RedoDB:

  * RedoDB is the world's first wait-free KV store;
  * Supports generic transactions passed on a lambda. These transactions can be over user provided (annotated) data types, or multiple operations in the KV store, or multiple operations over multiple KV store instances;
  * Both write and read-only operations have snapshots to work on. This means queries are non-abortable, similar to Multi-Version Concurrency Control (MVCC);
  * RedoDB does not use MVCC, it has its own novel concurrency control based on multiple replicas;
  * Can consume large amounts of persistent memory, but then again, so can MVCC;
  * This implementation is *not* production ready. This is research code, meant as a proof of concept that wait-free databases are possible and efficient;
  * Transactions in RedoDB are durable linearizable (serializable isolation);
  * Write transactions don't scale, but read-only transactions scale well, irrespective of how many write transactions exist;
  * The durability mechanism in RedoDB is not undo-log, nor redo-log, nor copy-on-write, it is multi-instace based (closer to Romulus);


More instructions to come soon...
