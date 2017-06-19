#NAME: Rohan Varma
#EMAIL: rvarm1@gmail.com
#ID: 111111111
#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#       8. avg log wait time
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#

# general plot parameters
set terminal png
set datafile separator ","

set title "Scalability-1: Throughput of synchronized lists"
set xlabel "Threads"
set ylabel "Throughput (operations/sec)"
set xrange [0.75:]
set logscale x 2
set logscale y 10
set output 'lab2b_1.png'

plot \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
        title 'list ins/lookup/delete w/mutex' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'list ins/lookup/delete w/spin' with linespoints lc rgb 'green'

set title "Scalability-2: Per-operation Times for Mutex-Protected List Operations"
set xlabel "Threads"
set ylabel "mean time/operation (ns)"
set xrange [0.75:]
set logscale x 2
set logscale y 10
set output 'lab2b_2.png'

plot \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):($7) \
        title 'completion time' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9]\\+,1000,1,' lab2b_list.csv" \
        using ($2):($8) \
        title 'wait for lock' with linespoints lc rgb 'green'

set title "Scalability-3: Correct Syncornization of Partitioned Lists"
set xrange [0.75:17]
set yrange [0.75:100]
set xlabel "Threads"
set ylabel "Successful Iterations"
set logscale y 10
set output 'lab2b_3.png'
plot \
    "< grep 'list-id-none,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3)\
	with points lc rgb "red" title "unprotected", \
    "< grep 'list-id-m,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "green" title "Mutex", \
    "< grep 'list-id-s,[0-9]\\+,[0-9]\\+,4,' lab2b_list.csv" using ($2):($3) \
	with points lc rgb "blue" title "Spin-Lock"

set title "Scalability-4: Throughput of Mutex-Synchronized Partitioned Lists"
set xlabel "Threads"
set ylabel "Throughput (operations/second)"
set logscale x 2
set logscale y
unset xrange
set xrange [0.75:]
unset yrange
set output 'lab2b_4.png'
plot \
     "< grep 'list-none-m,[0-9][2]\\?,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,4,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'orange', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,8,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-m,[0-9][2]\\?,1000,16,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'pink'

set title "Scalability-5: Throughput of Spin-Lock Synchronized Partitioned Lists"
set xlabel "Threads"
set ylabel "Throughput (operations/second)"
set logscale x 2
set logscale y
set output 'lab2b_5.png'
plot \
     "< grep 'list-none-s,[0-9][2]\\?,1000,1,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=1' with linespoints lc rgb 'red', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,4,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=4' with linespoints lc rgb 'orange', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,8,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=8' with linespoints lc rgb 'blue', \
     "< grep 'list-none-s,[0-9][2]\\?,1000,16,' lab2b_list.csv" \
        using ($2):(1000000000/($7)) \
	title 'lists=16' with linespoints lc rgb 'pink'
