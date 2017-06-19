# NAME: Ritam Sarmah, Rohan Varma
# EMAIL: rsarmah@ucla.edu, rvarm1@ucla.edu

.SILENT:
default:
	cp lab3b.py lab3b
	chmod +x lab3b lab3b.py

clean:
	rm -f *.tar.gz lab3b

dist: clean default
	tar -zcvf lab3b-111111111.tar.gz lab3b.py Makefile README
