# Name: Rohan Varma
# ID: 111111111
# email: rvarm1@ucla.edu
.SILENT:

default:
		gcc -g lab0.c -o lab0
clean:
	rm -rf lab0 *.txt *.tar.gz *.o

dist: default
	tar -czf lab0-111111111.tar.gz lab0.c Makefile README backtrace.png breakpoint.png

check: default badArgTest catchSegTest successExitTest outMatchesIn openErrorTest creatErrorTest

badArgTest:
	echo | ./lab0 --badarg &> /dev/null; \
	if [[ $$? -ne 1 ]]; then \
	echo "Test FAILED: bad argument returns wrong exit code"; \
	else \
	echo "Test PASSED: bad argument returns correct exit code"; \
	fi

catchSegTest:
	echo | ./lab0 --segfault --catch &> /dev/null; \
	if [[ $$? -ne 4 ]]; then \
	echo "Test FAILED: seg fault catch returns wrong exit code"; \
	else \
	echo "Test PASSED: catch returns expected exit code"; \
	fi

outMatchesIn:
	echo "Hello, World!" > in.txt; \
	./lab0 --input in.txt --output out.txt &> /dev/null; \
	cmp in.txt out.txt; \
	if [[ $$? -eq 0 ]]; then \
	echo "Test PASSED: outfile matches infile"; \
	else \
	echo "Test FAILED: outfile does not match infile"; \
	fi
	rm in.txt out.txt


successExitTest:
	echo "Hello, World!" > in.txt; \
	./lab0 --input in.txt --output out.txt &> /dev/null; \
	if [[ $$? -ne 0 ]]; then \
	echo "Test FAILED: program did not return with expected exit code"; \
	else \
	echo "Test PASSED: program returned with correct exit code"; \
	fi
	rm in.txt out.txt


openErrorTest:
	./lab0 --input dne.txt &> /dev/null; \
	if [[ $$? -ne 2 ]]; then \
	echo "Test FAILED: program does not return with exit code 2 when failing to open file"; \
	else \
	echo "Test PASSED: program exits with code 2 when failing to open file"; \
	fi

creatErrorTest:
	echo "Hello, World!" > in.txt; \
	touch out.txt; \
	chmod u-w out.txt; \
	./lab0 --input in.txt --output out.txt &> /dev/null; \
	if [[ $$? -ne 3 ]]; then \
	echo "Test FAILED: Program did not exit with correct exit code"; \
	else \
	echo "Test PASSED: program returned with correct exit code"; \
	fi
	chmod u=rwx out.txt; \
	rm in.txt out.txt
