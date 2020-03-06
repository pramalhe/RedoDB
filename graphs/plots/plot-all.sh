#!/bin/sh

for i in \
pcaption.gp \
psps-integer.gp \
pq-ll-volatile-pwb.gp \
pset-ll-10k.gp \
pset-tree-1m.gp \
pset-hash-1m.gp \
kvstore-1m.gp \
kvstore-10m.gp \
kvstore-pwb-recovery-10m.gp \
;
do
  echo "Processing:" $i
  gnuplot $i
  epstopdf `basename $i .gp`.eps
  rm `basename $i .gp`.eps
done


# pset-ll-1k.gp \
# pset-ll-10k.gp \
# pset-tree-1k.gp \
# pset-tree-1m.gp \
# pset-skiplist-1m.gp \
# pset-hash-1k.gp \
# pset-hash-1m.gp \
