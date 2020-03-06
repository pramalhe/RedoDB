echo "Run this on castor-1 to generate all the data files for the plots"
date > timestamp.txt

export PMEM_IS_PMEM_FORCE=1

make persistencyclean
#bin/pq-ll-enq-deq-ofwf
make persistencyclean
#bin/pq-ll-enq-deq-cxpuc
make persistencyclean
#bin/pq-ll-enq-deq-cxptm
make persistencyclean
#bin/pq-ll-enq-deq-redo
make persistencyclean
#bin/pq-ll-enq-deq-redotimed
make persistencyclean
bin/pq-ll-enq-deq-redoopt
make persistencyclean
#bin/pq-ll-enq-deq-pmdk
make persistencyclean
#bin/pq-ll-enq-deq-friedman

make persistencyclean
#bin/psps-integer-ofwf
make persistencyclean
#bin/psps-integer-cxptm
make persistencyclean
#bin/psps-integer-redo
make persistencyclean
#bin/psps-integer-redotimed
make persistencyclean
bin/psps-integer-redoopt
make persistencyclean
#bin/psps-integer-pmdk

make persistencyclean
#bin/pset-hash-1k-ofwf
make persistencyclean
#bin/pset-hash-1k-cxpuc
make persistencyclean
#bin/pset-hash-1k-cxptm
make persistencyclean
#bin/pset-hash-1k-redo
make persistencyclean
#bin/pset-hash-1k-redotimed
make persistencyclean
#bin/pset-hash-1k-redoopt
make persistencyclean
#bin/pset-hash-1k-pmdk

#make persistencyclean
#bin/pset-hash-10k-ofwf
#make persistencyclean
#bin/pset-hash-10k-cxpuc
#make persistencyclean
#bin/pset-hash-10k-cxptm
#make persistencyclean
#bin/pset-hash-10k-redo
#make persistencyclean
#bin/pset-hash-10k-redotimed
#make persistencyclean
#bin/pset-hash-10k-pmdk

make persistencyclean
#bin/pset-hash-1m-ofwf
#make persistencyclean
#bin/pset-hash-1m-cxpuc
make persistencyclean
#bin/pset-hash-1m-cxptm
make persistencyclean
#bin/pset-hash-1m-redo
make persistencyclean
#bin/pset-hash-1m-redotimed
make persistencyclean
bin/pset-hash-1m-redoopt
make persistencyclean
#bin/pset-hash-1m-pmdk

#make persistencyclean
#bin/pset-ll-1k-ofwf
#make persistencyclean
#bin/pset-ll-1k-cxpuc
#make persistencyclean
#bin/pset-ll-1k-cxptm
#make persistencyclean
#bin/pset-ll-1k-redo
#make persistencyclean
#bin/pset-ll-1k-redotimed
#make persistencyclean
#bin/pset-ll-1k-pmdk

make persistencyclean
#bin/pset-ll-10k-ofwf
make persistencyclean
#bin/pset-ll-10k-cxpuc
make persistencyclean
#bin/pset-ll-10k-cxptm
make persistencyclean
#bin/pset-ll-10k-redo
make persistencyclean
#bin/pset-ll-10k-redotimed
make persistencyclean
bin/pset-ll-10k-redoopt
make persistencyclean
#bin/pset-ll-10k-pmdk

make persistencyclean
#bin/pset-tree-1k-ofwf
make persistencyclean
#bin/pset-tree-1k-cxpuc
make persistencyclean
#bin/pset-tree-1k-cxptm
make persistencyclean
#bin/pset-tree-1k-redo
make persistencyclean
#bin/pset-tree-1k-redotimed
make persistencyclean
#bin/pset-tree-1k-redoopt
make persistencyclean
#bin/pset-tree-1k-pmdk

#make persistencyclean
#bin/pset-tree-10k-ofwf
#make persistencyclean
#bin/pset-tree-10k-cxpuc
#make persistencyclean
#bin/pset-tree-10k-cxptm
#make persistencyclean
#bin/pset-tree-10k-redo
#make persistencyclean
#bin/pset-tree-10k-redotimed
#make persistencyclean
#bin/pset-tree-10k-pmdk

make persistencyclean
#bin/pset-tree-1m-ofwf
#make persistencyclean
#bin/pset-tree-1m-cxpuc
make persistencyclean
#bin/pset-tree-1m-cxptm
make persistencyclean
#bin/pset-tree-1m-redo
make persistencyclean
#bin/pset-tree-1m-redotimed
make persistencyclean
bin/pset-tree-1m-redoopt
make persistencyclean
#bin/pset-tree-1m-pmdk

#make persistencyclean
#bin/pset-skiplist-1m-ofwf
#make persistencyclean
#bin/pset-skiplist-1m-cxpuc
#make persistencyclean
#bin/pset-skiplist-1m-cxptm
#make persistencyclean
#bin/pset-skiplist-1m-redo
#make persistencyclean
#bin/pset-skiplist-1m-redotimed
#make persistencyclean
#bin/pset-skiplist-1m-pmdk

touch dedicated.log
make persistencyclean
#bin/pset-tree-1m-dedicated-ofwf >> dedicated.log
#make persistencyclean
#bin/pset-tree-1m-dedicated-cxpuc >> dedicated.log
make persistencyclean
#bin/pset-tree-1m-dedicated-cxptm >> dedicated-cxptm.log
make persistencyclean
#bin/pset-tree-1m-dedicated-redo >> dedicated-redo.log
make persistencyclean
#bin/pset-tree-1m-dedicated-redotimed >> dedicated-redotimed.log
make persistencyclean
#bin/pset-tree-1m-dedicated-redoopt >> dedicated-redoopt.log
make persistencyclean
#bin/pset-tree-1m-dedicated-pmdk >> dedicated.log

echo > latency.log
make persistencyclean
#bin/pset-latency-ofwf >> latency.log
make persistencyclean
#bin/pset-latency-cxpuc >> latency.log
make persistencyclean
#bin/pset-latency-cxptm >> latency.log
make persistencyclean
#bin/pset-latency-redo >> latency.log
#make persistencyclean
#bin/pset-latency-redotimed >> latency.log
make persistencyclean
#bin/pset-latency-redoopt >> latency.log
#make persistencyclean
#bin/pset-latency-pmdk >> latency.log
#make persistencyclean
#bin/pset-latency-friedman >> latency.log
make persistencyclean
#bin/pset-latency-normopt >> latency.log


date >> timestamp.txt