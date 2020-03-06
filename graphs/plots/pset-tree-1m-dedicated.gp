set term postscript color eps enhanced 22
set output "pset-tree-1m-dedicated.eps"

set size 0.95,0.6

X=0.1
W=0.350
M=0.130

load "styles.inc"

set tmargin 11.2
set bmargin 2.5

set multiplot layout 1,2

unset key

set grid ytics

set xtics ("" 1, 2, "" 4, 8, 16, 32, 48, 64) nomirror out offset -0.25,0.5
set label at screen 0.25,0.03 center "Number of threads"
set label at screen 0.5,0.56 center "RB tree (1M keys), N readers + 2 updaters"
# set label at graph 1.0,1.075 center font "Helvetica-bold,18" "RB tree (1M keys), N readers + 2 updaters"

set xrange [1:64]

# First row

set lmargin at screen X
set rmargin at screen X+W

set ylabel offset 1.5,0 "Read-only op. ({/Symbol \264}10^6/s)"
set ytics 1 offset 0.5,0
set yrange [0:5]

set key at graph 0.50,0.99 samplen 1.5

plot \
    '../data/castor/pset-tree-1m-dedicated-cxptm.txt'         using 1:($2/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-cxredo.txt'        using 1:($2/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-cxredotimed.txt'   using 1:($2/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-redotimedhash.txt' using 1:($2/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-romlr.txt'         using 1:($2/1e6) with linespoints notitle ls 4 lw 3 dt (1,1), \
    '../data/castor/pset-tree-1m-dedicated-pmdk.txt'          using 1:($2/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pset-tree-1m-dedicated-ofwf.txt'          using 1:($2/1e6) with linespoints notitle ls 7 lw 3 dt (1,1)

unset ylabel

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
unset arrow
#set xtics ("" 1, 2, 4, 8, 16, 32, 64) nomirror out offset -0.25,0.5
set ylabel offset 1.5,0 "Mutative op. ({/Symbol \264}10^3/s)"
set label at screen 0.75,0.03 center "Number of threads"

set ytics 10 offset 0.5,0
set yrange [0:30]

#set label at graph 0.5,1.075 center font "Helvetica-bold,18" "RB tree (1M keys) + N readers"
set key at graph 0.99,0.99 samplen 1.5

# we divide by 1e8 because the data is in total number of tx and we want tx/second
plot \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($2/1e3) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($3/1e3) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($4/1e3) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($5/1e3) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($6/1e3) with linespoints notitle ls 4 lw 3 dt (1,1), \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($7/1e3) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pset-tree-1m-dedicated-writes.txt' using 1:($8/1e3) with linespoints notitle ls 7 lw 3 dt (1,1)

