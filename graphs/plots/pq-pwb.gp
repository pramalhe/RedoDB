set term postscript color eps enhanced 22
set output "pq-pwb.eps"

set size 0.95,0.6

S=0.2125
X=0.1
W=0.375
M=0.075

load "styles.inc"

set tmargin 10.5
set bmargin 3

# We can fit a second graph if need be (remove S "hack")
set multiplot layout 1,2

unset key

set grid ytics

set xtics ("" 1, "" 2, 4, "" 8, 16, 32, 40) nomirror out offset -0.25,0.5
set label at screen 0.5,0.04 center "Number of threads"

#set logscale x
set xrange [1:40]

# First row

set lmargin at screen S+X
set rmargin at screen S+X+W

set ylabel offset 1.5,0 "Average pwbs per op"
set yrange [0:14]
set ytics offset 1,0

#set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Linked list-based queues"
set key top left

#set label at graph 0.4,0.52 center font "Helvetica,18" "NormOpt"
#set arrow from graph 0.4,0.48 to graph 0.4,0.38 size screen 0.015,25 lw 3
#set label at graph 0.85,0.35 center font "Helvetica,18" "FHMP"
#set arrow from graph 0.85,0.31 to graph 0.85,0.14 size screen 0.015,25 lw 3

# The results in the .txt have the total number of pwbs and we know that the benchmarks always executes 10 million pairs => 20 million operations
plot \
    '../data/castor/pq-pwb-redotimedhash.txt' using 1:2 with linespoints notitle ls 9 lw 3 dt 1, \
    '../data/castor/pq-pwb-friedman.txt'      using 1:($2/2e7) with linespoints notitle ls 8 lw 3 dt 1, \
    '../data/castor/pq-pwb-normopt.txt'       using 1:($2/2e7) with linespoints notitle ls 10 lw 3 dt 1
