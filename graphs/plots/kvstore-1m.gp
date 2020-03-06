set term postscript color eps enhanced 22
set output "kvstore-1m.eps"

set size 0.95,0.55

X=0.08
W=0.24
M=0.06

load "styles.inc"

set tmargin 11.5
set bmargin 2.5

set grid ytics

set ylabel offset 1.5,0 "Operations ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
#set ylabel offset 2.0,0 "{/Symbol m}s/operation"
#set format y "10^{%T}"
set xtics ("" 1, "" 2, 4, 8, 16, 24, 32, 40) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set label at screen 0.5,0.03 center "Number of threads" font "Helvetica Condensed"

set multiplot layout 1,3

set yrange [0:9]
set ytics 2 offset 0.6,0 font "Helvetica Condensed"
set lmargin at screen X
set rmargin at screen X+W

#set label at graph 0.5,1.1 center "Read random"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Read random"
unset key

plot \
    '../data/castor/db-rocksdb-1m.txt'       using 1:($1/$4)  with linespoints notitle ls 11 lw 4 dt (1,1), \
    '../data/castor/db-redotimedhash-1m.txt' using 1:($1/$4)  with linespoints notitle ls 9  lw 4 dt 1
	
unset ylabel
#set ytics format "%g"

set yrange [0:9]
set ytics 2 offset 0.6,0 font "Helvetica Condensed"
set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
#set label at graph 0.5,1.1 center "Read while writing"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Read while writing"

plot \
    '../data/castor/db-rocksdb-1m.txt'       using 1:($1/$5)  with linespoints notitle ls 11 lw 4 dt (1,1), \
    '../data/castor/db-redotimedhash-1m.txt' using 1:($1/$5)  with linespoints notitle ls 9  lw 4 dt 1

unset ylabel
#set ytics format ""

set yrange [0:0.35]
set ytics 0.1 offset 0.6,0 font "Helvetica Condensed"
set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
#set label at graph 0.5,1.1 center "Overwrite"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Overwrite"

plot \
    '../data/castor/db-rocksdb-1m.txt'       using 1:($1/$3)  with linespoints notitle ls 11 lw 4 dt (1,1), \
    '../data/castor/db-redotimedhash-1m.txt' using 1:($1/$3)  with linespoints notitle ls 9  lw 4 dt 1

unset tics
unset border
unset xlabel
unset ylabel
unset label

set key at screen 0.925,0.13 samplen 2.0 bottom font "Helvetica Condensed"

plot [][0:1] \
    2 with linespoints title 'RedoDB'  ls 9  lw 4 dt 1, \
    2 with linespoints title 'RocksDB' ls 11 lw 4 dt (1,1)

unset multiplot
