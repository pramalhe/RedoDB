set term postscript color eps enhanced 22
set output "pq-ll-enq-deq.eps"

set size 0.75,0.6

S=0.2125
X=0.05
W=0.375
M=0.075

load "styles.inc"

set tmargin 10.5
set bmargin 3

# We can fit a second graph if need be (remove S "hack")
set multiplot layout 1,2

unset key

set grid ytics

set xtics ("" 1, "" 2, 4, "" 8, 16, "" 24, 32, 40) nomirror out offset -0.25,0.5
set label at screen 0.45,0.04 center "Number of threads"

#set logscale x
set xrange [1:40]

# First row

set lmargin at screen S+X
set rmargin at screen S+X+W

set ylabel offset 1.5,0 "Operations ({/Symbol \264}10^6/s)"

#set yrange [0:0.5]    # scale for plot without fat node queue
set yrange [0:1.2]   # scale for plot with fat node queue
set ytics offset 0.25,0

#set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Linked list-based queues"
set key top left

set label at graph 0.4,0.52 center font "Helvetica,18" "NormOpt"
set arrow from graph 0.4,0.48 to graph 0.4,0.38 size screen 0.015,25 lw 3
set label at graph 0.85,0.35 center font "Helvetica,18" "FHMP"
set arrow from graph 0.85,0.31 to graph 0.85,0.14 size screen 0.015,25 lw 3

plot \
    '../data/castor/pq-ll-enq-deq-cxpuc.txt'             using 1:($2/1e6) with linespoints notitle ls 1 lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-cxptm.txt'             using 1:($2/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-cxredo.txt'            using 1:($2/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-cxredotimed.txt'       using 1:($2/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-redotimedhash.txt'     using 1:($2/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pq-ll-enq-deq-pmdk.txt'              using 1:($2/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pq-ll-enq-deq-ofwf.txt'              using 1:($2/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
	'../data/castor/pq-ll-enq-deq-friedman.txt'          using 1:($2/1e6) with linespoints notitle ls 8 lw 3 dt (1,1), \
    '../data/castor/pq-ll-enq-deq-normopt.txt'           using 1:($2/1e6) with linespoints notitle ls 10 lw 3 dt (1,1)
    

#    '../data/castor/pq-ll-enq-deq-redotimedhash-fat.txt' using 1:($2/1e6) with linespoints notitle ls 9 lw 3 dt (1,2), \
#'../data/castor/pq-ll-enq-deq-romlr.txt'             using 1:($2/1e6) with linespoints notitle ls 4 lw 3 dt (1,1), \