set pointsize 2

# BLUE   -> #0066CC (light: #0099FF - dark: #003366)
# RED    -> #993333 (dark: #8B0000)
# GREEN  -> #669900 (light: #00CC66)
# PURPLE -> #663399
# ORANGE -> #CC6633 (light: #FFCC00)
# PINK   -> #FC0FC0
#
# https://blog.twei7.com/gnuplot/

# CX-PUC
set style line 1 lt 1 lc rgb "#FF9900" lw 2 pt 9  ps 1.5 dt 1
# CX-PTM
set style line 2 lt 1 lc rgb "#CC0000" lw 2 pt 11 ps 1.5 dt 1
# Redo
set style line 3 lt 1 lc rgb "#0099FF" lw 2 pt 7  ps 1.5 dt 1
# RomulusLR
set style line 4 lt 1 lc rgb "#669900" lw 2 pt 5  ps 1 dt 1
# RedoTimed
set style line 5 lt 1 lc rgb "#0066CC" lw 2 pt 4  ps 1.25 dt 1
# PMDK
set style line 6 lt 1 lc rgb "#993333" lw 2 pt 2  ps 1.25 dt 1
# OFWF
set style line 7 lt 1 lc rgb "#669900" lw 2 pt 3  ps 1.25 dt 1
# Leveldb/Friedman queue
set style line 8 lt 1 lc rgb "#000000" lw 2 pt 15  ps 1.25 dt 1
# RedoOpt
set style line 9 lt 1 lc rgb "#003399" lw 2 pt 3  ps 1.75 dt 1
# NormOpt
set style line 10 lt 1 lc rgb "#000000" lw 2 pt 3  ps 1.25 dt 1
# RocksDB
set style line 11 lt 1 lc rgb "#993333" lw 2 pt 2  ps 1.25 dt 1
pat1 = 'fs solid 0.5 lc rgb "#FFCC00"'
pat2 = 'fs solid 0.5 lc rgb "#00CC66"'
pat3 = 'fs solid 0.7 lc rgb "#0066CC"'
pat4 = 'fs solid 1.0 lc rgb "#993333"'
pat5 = 'fs solid 1.0 lc rgb "#333333"'
#pat5 = 'fs pattern 2 lc rgb "#0066CC"'
#pat6 = 'fs pattern 6 lc rgb "#0066CC"'
#pat5 = 'fs pattern 2 lc rgb "#000000"'
#pat6 = 'fs pattern 6 lc rgb "#000000"'

#pat1 = 'fs solid 0.0  lc rgb "#000000"'
#pat2 = 'fs solid 0.3  lc rgb "#000000"'
#pat3 = 'fs solid 0.65 lc rgb "#000000"'
#pat4 = 'fs solid 1.0  lc rgb "#000000"'
#pat5 = 'fs pattern 2  lc rgb "#000000"'
#pat6 = 'fs pattern 6  lc rgb "#000000"'
#pat7 = 'fs solid 0.5  lc rgb "#000000"'
