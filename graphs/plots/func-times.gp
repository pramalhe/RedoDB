set term postscript color eps enhanced 22
set output "func-times.eps"

set size 0.95,0.6

X=0.1
W=0.2
M=0.01

load "styles.inc"

set tmargin 11.0
set bmargin 2.5

set grid ytics

set ylabel offset 0.5,0 "Avg time per func\n{/=16 (vs. RedoOpt)}"
set label at screen 0.5,0.03 center "updateTx, applyLog, flush, copy, lambdas, sleep"

set yrange[0:8]
set ytics 1 nomirror offset 0.5,0

set noxtics

set style data histogram
set style histogram clustered gap 2
set style fill solid border 0
set boxwidth 0.95 relative
set offset -0.6,-0.6,-0.6,0.6

set multiplot layout 1,4

set lmargin at screen X
set rmargin at screen X+W

set label at graph 0.5,1.1 center "hash 4T"
set key at graph 0.0,1.15 Left left reverse invert samplen 0.5 font ",16"

#set style fill solid 1.00 noborder
#set style increment default
#set style histogram clustered title textcolor lt -1 font ",11"  offset character 2, -2

updatetx = `head -2 '../data/castor/func-times-hash-4.txt' | tail -1 | awk '{print $4}'`
set label sprintf("%d",updatetx) at 0.5,1.1 font ",12" rotate by 90



plot '../data/castor/func-times-hash-4.txt' using ($4/updatetx) notitle ls 9,\
     "" using ($2/updatetx):xtic(1) notitle ls 3,\
     "" using ($3/updatetx) notitle ls 5,\
     "" using ($5/updatetx) notitle ls 7

unset ylabel
set ytics format ""

set lmargin at screen X+(W+M)
set rmargin at screen X+(W+M)+W

unset label
set label at graph 0.5,1.1 center "hash 16T"

updatetx = `head -2 '../data/castor/func-times-hash-16.txt' | tail -1 | awk '{print $4}'`
set label sprintf("%d",updatetx) at 0.5,1.1 font ",12" rotate by 90

plot '../data/castor/func-times-hash-16.txt' using ($4/updatetx) notitle ls 9,\
     "" using ($2/updatetx):xtic(1) notitle ls 3,\
     "" using ($3/updatetx) notitle ls 5,\
     "" using ($5/updatetx) notitle ls 7

unset xlabel

set lmargin at screen X+2*(W+M)
set rmargin at screen X+2*(W+M)+W

unset label
set label at graph 0.5,1.1 center "tree 4T"

updatetx = `head -2 '../data/castor/func-times-tree-4.txt' | tail -1 | awk '{print $4}'`
set label sprintf("%d",updatetx) at 0.5,1.1 font ",12" rotate by 90

plot '../data/castor/func-times-tree-4.txt' using ($4/updatetx) notitle ls 9,\
     "" using ($2/updatetx):xtic(1) notitle ls 3,\
     "" using ($3/updatetx) notitle ls 5,\
     "" using ($5/updatetx) notitle ls 7
	
unset xlabel

set lmargin at screen X+3*(W+M)
set rmargin at screen X+3*(W+M)+W

unset label
set label at graph 0.5,1.1 center "tree 16T"

updatetx = `head -2 '../data/castor/func-times-tree-16.txt' | tail -1 | awk '{print $4}'`
set label sprintf("%d",updatetx) at 0.5,1.1 font ",12" rotate by 90

plot '../data/castor/func-times-tree-16.txt' using ($4/updatetx) notitle ls 9,\
     "" using ($2/updatetx):xtic(1) notitle ls 3,\
     "" using ($3/updatetx) notitle ls 5,\
     "" using ($5/updatetx) notitle ls 7

unset label

unset multiplot
