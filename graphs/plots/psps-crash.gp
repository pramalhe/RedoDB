set term postscript color eps enhanced 22
set output "psps-crash.eps"

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

set xtics (1000, 1000000, 1000000000) nomirror out offset -0.25,0.5
set label at screen 0.5,0.04 center "array size"

set logscale x
set xrange [1000:1000000000]

# First row

set lmargin at screen S+X
set rmargin at screen S+X+W


set ylabel offset 1,0 "time (seconds)"
set ytics 1 offset 0.5,0
set yrange [0:4]

#set label at graph 0.5,1.075 center font "Helvetica-bold,18" "Swapping item from q to q"
set key at graph 0.99,0.99 samplen 1.5

plot \
    '../data/castor/psps-crash-redotimedhash.txt' using 1:($2/1e3) with linespoints notitle ls 9 lw 3 dt 1, \
    '../data/castor/psps-crash-ofwf.txt'   using 1:($2/1e3) with linespoints notitle ls 7 lw 3 dt (1,1)
