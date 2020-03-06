set term postscript color eps enhanced 22
set output "psps-integer.eps"

set size 0.95,1.05

X=0.1
W=0.26
M=0.025

load "styles.inc"

set tmargin 0
set bmargin 3

set multiplot layout 2,3

unset key

set grid ytics

set xtics ("" 1, "" 2, 4, 8, 16, 24, 32, 40) nomirror out offset -0.25,0.5 font "Helvetica Condensed"
set label at screen 0.5,0.04 center "Number of threads" font "Helvetica Condensed"
#set label at screen 0.5,1.075 center "SPS integer swap"

#set logscale x
set xrange [1:40]

# First row

set lmargin at screen X
set rmargin at screen X+W

#set ylabel offset 1,0 "Swaps ({/Symbol \264}10^6/s)"
set ylabel offset 2.5,0 "Tx ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:0.6]

set label at graph 0.5,1.075 center font "Helvetica-bold,18" "1 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($2/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($2/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($2/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($2/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($2/1e6) with linespoints notitle ls 6 lw 3 dt (1,1), \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($2/1e6) with linespoints notitle ls 7 lw 3 dt (1,1)

unset ylabel
set ytics format ""

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:1.1]
set style textbox opaque noborder fillcolor rgb "white"
set label at first 1,1.1 front boxed left offset -0.5,0 "1.1" font "Helvetica Condensed"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "4 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($3/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($3/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($3/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($3/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($3/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($3/1e6) with linespoints notitle ls 6 lw 3 dt (1,1)

set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:1.2]
set style textbox opaque noborder fillcolor rgb "white"
set label at first 1,1.2 front boxed left offset -0.5,0 "1.2" font "Helvetica Condensed"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "8 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($4/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($4/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($4/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($4/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($4/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($4/1e6) with linespoints notitle ls 6 lw 3 dt (1,1)

# Second row

set lmargin at screen X
set rmargin at screen X+W

#set ylabel offset 2.0,0 "Swaps ({/Symbol \264}10^6/s)"
set ylabel offset 1.5,0 "Tx ({/Symbol \264}10^6/s)" font "Helvetica Condensed"
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set format y "{%g}"
set yrange [0:1.2]

unset label
#set label at first 0,2.3 front boxed left offset -2.3,-0.2 "2.3"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "16 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($5/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($5/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($5/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($5/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($5/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($5/1e6) with linespoints notitle ls 6 lw 3 dt (1,1)

unset ylabel
set ytics format ""

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:1.2]
set style textbox opaque noborder fillcolor rgb "white"
#set label at first 1,1.5 front boxed left offset -0.5,0.5 "1.6"
#set label at first 64,1.6 front boxed left offset -1.5,0 "1.6"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "32 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($6/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($6/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($6/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($6/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($6/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($6/1e6) with linespoints notitle ls 6 lw 3 dt (1,1)

set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
set ytics 0.2 offset 0.5,0 font "Helvetica Condensed"
set yrange [0:1.2]
set style textbox opaque noborder fillcolor rgb "white"
#set label at first 64,1.6 front boxed left offset -1.5,0 "1.6"
set label at graph 0.5,1.075 center font "Helvetica-bold,18" "64 swaps/tx"

plot \
    '../data/castor/psps-integer-cxptm.txt'     using 1:($7/1e6) with linespoints notitle ls 2 lw 3 dt 1,     \
    '../data/castor/psps-integer-redo.txt'      using 1:($7/1e6) with linespoints notitle ls 3 lw 3 dt 1,     \
    '../data/castor/psps-integer-redotimed.txt' using 1:($7/1e6) with linespoints notitle ls 5 lw 3 dt 1,     \
    '../data/castor/psps-integer-redoopt.txt'   using 1:($7/1e6) with linespoints notitle ls 9 lw 3 dt 1,     \
    '../data/castor/psps-integer-ofwf.txt'      using 1:($7/1e6) with linespoints notitle ls 7 lw 3 dt (1,1), \
    '../data/castor/psps-integer-pmdk.txt'      using 1:($7/1e6) with linespoints notitle ls 6 lw 3 dt (1,1)

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
#    2 with linespoints title 'PMDK'     ls 6   
#    2 with linespoints title 'OF-WF'    ls 5, \
    