#NAME: Rohan Varma
#EMAIL: rvarm1@ucla.edu
#ID: 111111111
.SILENT:

default: build

build: tls tcp

tcp:
	gcc -g -std=gnu99 -pthread -lm -lmraa lab4c_tcp.c -o lab4c_tcp

tls:
	gcc -g -pthread -std=gnu99 -lm -lmraa -lssl -lcrypto lab4c_tls.c -o lab4c_tls


clean:
	rm -f *.tar.gz lab4c_tcp lab4c_tls

check: dist build
	./lab4c_tcp --id=111111111 18000
	./lab4c_tls --id=111111111 19000
	./check.sh 111111111
	
dist:
	tar -zcvf lab4c-111111111.tar.gz *.c README Makefile check.sh





	
