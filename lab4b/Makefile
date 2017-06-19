#NAME: Rohan Varma
#EMAIL: rvarm1@ucla.edu
#ID: 111111111
.SILENT:
default: build
build:
	gcc -g -lmraa -lm -pthread lab4b.c -o lab4b

check: build
	chmod u=rwx my_check.sh
	./my_check.sh

clean:
	rm -f lab4b *.tar.gz
	
dist:
	tar -zcvf lab4b-111111111.tar.gz *.c README Makefile *.sh





	
