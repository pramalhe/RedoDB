# This plot combines pq-ll-enq-deq-volatile and pq-ll-pwb
set term postscript color eps enhanced 22
set output "pq-ll-volatile-pwb.eps"

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

set ylabel offset 2.25,0 "Operations ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:1.2]

set label at graph 0.45,0.57 center font "Helvetica,18" "NormOpt"
set arrow from graph 0.45,0.52 to graph 0.45,0.28 size screen 0.015,25 lw 3
set label at graph 0.85,0.60 center font "Helvetica,18" "FHMP"
set arrow from graph 0.85,0.55 to graph 0.85,0.41 size screen 0.015,25 lw 3


# set label at graph 0.5,1.075 center font "Helvetica-bold,18" "FHMP and NormOpt volatile alloc"
set key top left

plot \
    '../data/castor/pq-ll-enq-deq-redoopt.txt'           using 1:($2/1e6) with linespoints notitle ls 9  lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-friedman-volatile.txt' using 1:($2/1e6) with linespoints notitle ls 8  lw 3 dt (1,1), \
    '../data/castor/pq-ll-enq-deq-pmdk.txt'              using 1:($2/1e6) with linespoints notitle ls 6  lw 3 dt (1,1), \
    '../data/castor/pq-ll-enq-deq-ofwf.txt'              using 1:($2/1e6) with linespoints notitle ls 7  lw 3 dt (1,1), \
    '../data/castor/pq-ll-enq-deq-normopt-volatile.txt'  using 1:($2/1e6) with linespoints notitle ls 10 lw 3 dt (1,1)

unset ylabel
unset label
unset arrow
#unset ytics
set ytics format ""

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
unset ylabel
set ylabel offset 0.25,0 "Avg. pwbs/op" font "Helvetica Condensed"
set format y "%g"
set yrange [0:16]
set ytics 2 offset 0.5,0 font "Helvetica Condensed"
set key top left

set label at graph 0.45,0.57 center font "Helvetica,18" "NormOpt"
set arrow from graph 0.45,0.62 to graph 0.45,0.79 size screen 0.015,25 lw 3
set label at graph 0.85,0.68 center font "Helvetica,18" "FHMP"
set arrow from graph 0.85,0.63 to graph 0.85,0.47 size screen 0.015,25 lw 3

plot \
    '../data/castor/pq-pwb-redoopt.txt'       using 1:2 with linespoints notitle ls 9  lw 3 dt 1, \
    '../data/castor/pq-pwb-friedman.txt'      using 1:2 with linespoints notitle ls 8  lw 3 dt (1,1), \
    '../data/castor/pq-pwb-pmdk.txt'          using 1:2 with linespoints notitle ls 6  lw 3 dt (1,1), \
    '../data/castor/pq-pwb-ofwf.txt'          using 1:2 with linespoints notitle ls 7  lw 3 dt (1,1), \
    '../data/castor/pq-pwb-normopt.txt'       using 1:2 with linespoints notitle ls 10 lw 3 dt (1,1)
