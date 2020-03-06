set term postscript color eps enhanced 22
set output "pset-hash-1m.eps"

set size 0.95,0.6

X=0.1
W=0.26
M=0.025

load "styles.inc"

set tmargin 11.8
set bmargin 2.5

set multiplot layout 1,3

unset key

set grid ytics

set xtics ("" 1, "" 2, 4, 8, 16, 24, 32, 40) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set label at screen 0.5,0.03 center "Number of threads" font "Helvetica Condensed"
set label at screen 0.5,0.565 center "Persistent resizable hash set with 10^{6} keys"

set xrange [1:40]

# First row

set lmargin at screen X
set rmargin at screen X+W

set ylabel offset 0.75,0 "Operations ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set format y "%g"
set yrange [0:0.7]
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "100% updates"

set key at graph 0.99,0.99 samplen 1.5

plot \
    '../data/castor/pset-hash-1m-cxptm.txt'         using 1:($2/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredo.txt'        using 1:($2/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredotimed.txt'   using 1:($2/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-redotimedhash.txt' using 1:($2/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-pmdk.txt'          using 1:($2/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pset-hash-1m-ofwf.txt'          using 1:($2/1e6) with linespoints notitle ls 7 lw 3 dt (1,1)

#    '../data/pset-hash-1m-cxpuc.txt'       using 1:($2/1e6) with linespoints notitle ls 1 lw 3 dt 1,     \

unset ylabel
set ytics format ""

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
set ytics 1 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:4.5]
set style textbox opaque noborder fillcolor rgb "white"
set label at first 1,4.5 front boxed left offset -0.5,0 "4.5" font "Helvetica Condensed"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "10% updates"

plot \
    '../data/castor/pset-hash-1m-cxptm.txt'         using 1:($3/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredo.txt'        using 1:($3/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredotimed.txt'   using 1:($3/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-redotimedhash.txt' using 1:($3/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-pmdk.txt'          using 1:($3/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pset-hash-1m-ofwf.txt'          using 1:($3/1e6) with linespoints notitle ls 7 lw 3 dt (1,1)

#    '../data/pset-hash-1m-cxpuc.txt'       using 1:($3/1e6) with linespoints notitle ls 1 lw 3 dt 1,     \

set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
set ytics 5 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:12]
set style textbox opaque noborder fillcolor rgb "white"
set label at first 1,12 front boxed left offset -0.5,0 "12" font "Helvetica Condensed"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "1% updates"

plot \
    '../data/castor/pset-hash-1m-cxptm.txt'         using 1:($4/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredo.txt'        using 1:($4/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-cxredotimed.txt'   using 1:($4/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-redotimedhash.txt' using 1:($4/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/pset-hash-1m-pmdk.txt'          using 1:($4/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/pset-hash-1m-ofwf.txt'          using 1:($4/1e6) with linespoints notitle ls 7 lw 3 dt (1,1)

#    '../data/pset-hash-1m-cxpuc.txt'       using 1:($4/1e6) with linespoints notitle ls 1 lw 3 dt 1,     \

# Second row
unset tics
unset border
unset xlabel
unset ylabel
unset label

#set key at screen 0.92,0.20 samplen 2.0 bottom
#plot [][0:1] \
#    2 with linespoints title 'CX-PUC'   ls 1, \
#    2 with linespoints title 'CX-PTM'   ls 2, \
#    2 with linespoints title 'CX-Redo'  ls 3, \
#    2 with linespoints title 'RomLR'    ls 4, \
#    2 with linespoints title 'OF-WF'    ls 5, \
#    2 with linespoints title 'PMDK'     ls 6       
    