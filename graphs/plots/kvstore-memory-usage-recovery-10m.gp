set term postscript color eps enhanced 22
set output "kvstore-memory-usage-recovery-10m.eps"

set size 0.95,0.55

X=0.085
W=0.23
M=0.075

load "styles.inc"

set tmargin 11
set bmargin 2.5

set grid ytics

set ylabel offset 3.2,0 font "Helvetica Condensed" "Volatile Memory (GB)"
set xtics ("" 1, "" 2, 4, 8, 16, 24, 32, 40) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set ytics 1 offset 0.5,0
set xlabel offset 0,1 font "Helvetica Condensed" "Number of threads"

set multiplot layout 1,3

set ytics 0.5 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:3.2]
set lmargin at screen X
set rmargin at screen X+W

unset key

plot \
    '../data/castor/db-memory-usage.txt' using 1:($2/(1024*1024)) with linespoints notitle ls 11 lw 3 dt (1,1), \
    '../data/castor/db-memory-usage.txt' using 1:($4/(1024*1024)) with linespoints notitle ls 9  lw 3 dt 1
	
unset ylabel
#set ytics format "%g"

set ylabel offset 2.0,0 font "Helvetica Condensed" "Persistent Memory (GB)"
set yrange [0:6]
set ytics 1 offset 0.5,0 font "Helvetica Condensed"
set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W
set xlabel offset 0,1 font "Helvetica Condensed" "Number of threads"

unset label

plot \
    '../data/castor/db-memory-usage.txt' using 1:($3/(1024*1024)) with linespoints notitle ls 11 lw 3 dt (1,1), \
    '../data/castor/db-memory-usage.txt' using 1:($5/(1024*1024)) with linespoints notitle ls 9  lw 3 dt 1

unset ylabel
#set ytics format ""

set format x "10^{%T}"
set xtics (10000, 100000, 1000000, 10000000, 10000000) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set ylabel offset 2.2,0 font "Helvetica Condensed" "Recovery time (s)"
set yrange [0:2.5]
set ytics 1 offset 0.6,0 font "Helvetica Condensed"
set mytics 2
set grid mytics
set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W
set logscale x
set xrange [10000:10000000]
#set label at screen 0.6,0.03 center "array size"
set xlabel offset 0,1 font "Helvetica Condensed" "DB size (keys)"

unset label
#set label at graph 0.5,1.1 center "Recovery (sec)"

plot \
    '../data/castor/db-crash-redodb.txt'  using 1:($2/1e3) with linespoints notitle ls 9  lw 4 dt 1, \
    '../data/castor/db-crash-rocksdb.txt' using 1:($2/1e3) with linespoints notitle ls 11 lw 4 dt (1,1)

unset tics
unset border
unset xlabel
unset ylabel
unset label

set key at screen 0.28,0.41 samplen 2.0 bottom font "Helvetica Condensed"

plot [][0:1] \
    2 with linespoints title 'RedoDB'  ls 9  lw 4 dt 1, \
    2 with linespoints title 'RocksDB' ls 11 lw 4 dt (1,1)

unset multiplot
