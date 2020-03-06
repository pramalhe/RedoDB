set term postscript color eps enhanced 22
set output "pcaption.eps"

set size 0.95,0.20

load "styles.inc"

unset tics
unset border
unset xlabel
unset ylabel
unset label

set object 1 rectangle from screen 0.01,0.01 to screen 0.94,0.19 fillcolor rgb "white" dashtype (2,3) behind
set label at screen 0.5,0.16 center "{/Helvetica-bold Legend for plots} (all graphs of {\247}7)"

set key at screen 0.46,0.07 samplen 1.5 maxrows 2 width 0.0 center
plot [][0:1] \
    2 with linespoints title 'CX-PUC'        ls 1 lw 3 dt 1, \
    2 with linespoints title 'CX-PTM'        ls 2 lw 3 dt 1, \
    2 with linespoints title 'Redo'          ls 3 lw 3 dt 1, \
    2 with linespoints title 'RedoTimed'     ls 5 lw 3 dt 1, \
	2 with linespoints title 'RedoOpt'       ls 9 lw 3 dt 1, \
    2 with linespoints title 'PMDK'          ls 6 lw 3 dt (1,1), \
    2 with linespoints title 'OneFileWF'     ls 7 lw 3 dt (1,1)
