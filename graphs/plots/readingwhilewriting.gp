set term postscript color eps enhanced 22
set output "readingwhilewriting.eps"

set size 0.95,0.6

X=0.08
W=0.23
M=0.07

load "styles.inc"

set tmargin 11.0
set bmargin 2.5

set grid ytics

set ylabel offset 1.5,0 "{/Symbol m}s/operation"
#set format y "10^{%T}"
set xtics ("" 1, "" 2, 4, 8, 16, 24, 32) nomirror out offset -0.25,0.5
set ytics 1 offset 0.5,0
set label at screen 0.5,0.03 center "Number of reader threads"

set multiplot layout 1,3

set yrange [0:8]
set lmargin at screen X
set rmargin at screen X+W

set label at graph 0.5,1.1 center "Read while writing"
unset key

plot \
    '../data/readingwhilewriting.txt' using 1:2  with linespoints notitle ls 5 lw 2 dt 1, \
    '../data/readingwhilewriting.txt' using 1:3  with linespoints notitle ls 4 lw 2 dt 1, \
    '../data/readingwhilewriting.txt' using 1:4  with linespoints notitle ls 3 lw 2 dt 1
	
unset ylabel
#set ytics format "%g"

set yrange [0:0.8]
set ytics 0.2 offset 0.5,0
set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
set label at graph 0.5,1.1 center "Read random"

plot \
    '../data/readrandom.txt' using 1:2 with linespoints notitle ls 5 lw 2 dt 1, \
    '../data/readrandom.txt' using 1:3 with linespoints notitle ls 4 lw 2 dt 1, \
    '../data/readrandom.txt' using 1:4 with linespoints notitle ls 3 lw 2 dt 1

unset ylabel
#set ytics format ""

set yrange [0:110]
set ytics 20 offset 0.5,0
set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
set label at graph 0.5,1.1 center "Fill sync"
	
plot \
	'../data/fillsync.txt' using 1:2  with linespoints notitle ls 5 lw 2 dt 1, \
    '../data/fillsync.txt' using 1:3  with linespoints notitle ls 4 lw 2 dt 1, \
    '../data/fillsync.txt' using 1:4  with linespoints notitle ls 3 lw 2 dt 1

unset tics
unset border
unset xlabel
unset ylabel
unset label

set key at screen 0.60,0.12 samplen 2.0 bottom opaque

plot [][0:1] \
    2 with linespoints title 'RedoDB'  ls 5, \
    2 with linespoints title 'RomDB'   ls 4, \
    2 with linespoints title 'LevelDB' ls 3

#set key at screen 0.62,0.408 samplen 2.0 opaque bottom

#plot [][0:1] \
#    2 title 'Read TX'  lc rgb "black" lw 2 dt 1, \
#    2 title 'Write TX' lc rgb "black" lw 8 dt (1,1)

unset multiplot
