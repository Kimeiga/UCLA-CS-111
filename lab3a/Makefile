# NAME: Ritam Sarmah, Rohan Varma
# EMAIL: rsarmah@ucla.edu, rvarm1@ucla.edu

CC = gcc
CFLAGS = -std=gnu11

OBJECTS = README lab3a.c ext2_fs.h Makefile

.SILENT:
default:
	$(CC) $(CFLAGS) -o lab3a lab3a.c
	
clean:
	rm -f results* *.o *.diff lab3a *.tar.gz

dist: clean default
	tar -zcvf lab3a-111111111.tar.gz $(OBJECTS)
