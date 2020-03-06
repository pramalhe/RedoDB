set term postscript color eps enhanced 22
set output "kvstore-fillrandom-pwb-10m.eps"

set size 0.95,0.6

X=0.1
W=0.35
M=0.125

load "styles.inc"

set tmargin 10.0
set bmargin 3

set multiplot layout 1,2

set grid ytics

set xtics ("" 1, "" 2, 4, 8, 16, 24, 32, 40) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set label at screen 0.5,0.04 center "Number of threads" font "Helvetica Condensed"
set xrange [1:40]

set lmargin at screen X
set rmargin at screen X+W

set ylabel offset 2.5,0 "Operations ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
set yrange [0:0.35]
set ytics 0.1 offset 0.6,0 font "Helvetica Condensed"
set key top left

plot \
    '../data/castor/db-rocksdb-10m.txt'       using 1:($1/$2)  with linespoints notitle ls 11 lw 4 dt (1,1), \
    '../data/castor/db-redotimedhash-10m.txt' using 1:($1/$2)  with linespoints notitle ls 9  lw 4 dt 1

unset ylabel
unset label

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

set ylabel offset 0.25,0 "Avg. pwbs/op" font "Helvetica Condensed"
set format y "%g"
set yrange [0:8]
set ytics 2 offset 0.6,0 font "Helvetica Condensed"

# We divide by 10M (the number of keys) and devide by the number of threads because the data values are for the total and we want the average per op
plot \
    '../data/castor/db-rocksdb-pwb-10m.txt'       using 1:($2/(1e7*$1))  with linespoints notitle ls 11 lw 4 dt (1,1), \
    '../data/castor/db-redotimedhash-pwb-10m.txt' using 1:($2/(1e7*$1))  with linespoints notitle ls 9  lw 4 dt 1


#set key top left
set key at screen 0.60,0.15 samplen 2.0 bottom opaque

plot [][0:1] \
    2 with linespoints title 'RedoDB'  ls 9  lw 4 dt 1, \
    2 with linespoints title 'RocksDB' ls 11 lw 4 dt (1,1)
    