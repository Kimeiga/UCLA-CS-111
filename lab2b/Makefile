#NAME: Rohan Varma
#EMAIL: rvarm1@gmail.com
#ID: 111111111[rohan@lnxsrv09
default: build

build:
	gcc -pthread -g lab2_list.c SortedList.c -o lab2_list

clean:
	rm -rf lab2_list *.o *.csv *.png *.perf profile*

tests: clean build
	for n in 1 4 8 12 16 24 ; do \
		./lab2_list --iterations=1000 --sync=m --threads=$$n >> lab2b_list.csv ; \
		./lab2_list --iterations=1000 --sync=s --threads=$$n >> lab2b_list.csv ; \
	done

	for n in 1 4 8 12 16 ; do \
		for i in 1 2 4 8 16 ; do \
			./lab2_list --yield=id --lists=4 --threads=$$n --iterations=$$i >> lab2b_list.csv || true ; \
		done ; \
	done
	for n in 1 4 8 12 16 ; do \
		for i in 10 20 40 80 ; do \
			./lab2_list --yield=id --lists=4 --threads=$$n --iterations=$$i --sync=m >> lab2b_list.csv ; \
			./lab2_list --yield=id --lists=4 --threads=$$n --iterations=$$i --sync=s >> lab2b_list.csv ; \
		done ; \
	done

	for n in 1 2 4 8 12 ; do \
		for l in 4 8 16 ; do \
			./lab2_list --sync=m --iterations=1000 --threads=$$n --lists=$$l >> lab2b_list.csv ; \
			./lab2_list --sync=s --iterations=1000 --threads=$$n --lists=$$l >> lab2b_list.csv ; \
		done ; \
	done


graphs: tests
	gnuplot lab2_list.gp

profile: clean build
	-rm -f ./raw.gperf
	LD_PRELOAD=/u/cs/ugrad/rohan/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	pprof --text ./lab2_list ./raw.gperf > profile.out
	pprof --list=spin_lock_and_time ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf

dist: clean build graphs profile
	tar -zcvf lab2b-111111111.tar.gz *.c *.h README Makefile *.png *.csv *.out *.gp
