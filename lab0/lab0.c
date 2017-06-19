/*
Name: Rohan Varma
ID: 111111111
email: rvarm1@ucla.edu
 */

#include <stdio.h>     /* for printf */
#include <stdlib.h>    /* for exit */
#include <getopt.h>
#include <signal.h> /* for SIGSEV */
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

int errno;

void catch_segfault(int num) {
    if (num == SIGSEGV) {
        fprintf(stderr, "Seg fault caught: error = %d, message =  %s \n", errno, strerror(errno));
        exit(4);
    }
}

void cause_seg_fault() {
    // cause a seg fault by dereferencing a nullptr
    char *nptr = NULL;
    *nptr = 'h';
}

void print_ops_and_exit() {
    printf("Usage: lab0 --input filename --output filename [sc] \n");
    exit(1);
}

int main(int argc, char **argv) {
    static struct option long_options[] = {
        {"input", required_argument, 0, 'i'},
        {"output", required_argument, 0, 'o'},
        {"segfault", no_argument, 0, 's'},
        {"catch", no_argument, 0, 'c'},
        {0, 0, 0, 0}
    };
    char *infile = NULL;
    char *outfile = NULL;
    int seg = 0;
    int catch = 0;
    int opt, input_fd, output_fd;
    while( (opt = getopt_long(argc, argv, "i:o:sc", long_options, NULL)) != -1) {
        switch (opt) {
        case 'i':
            infile = optarg;
            break;
        case 'o':
            outfile = optarg;
            break;
        case 's':
            seg = 1;
            break;
        case 'c':
            catch = 1;
            break;
        default:
            print_ops_and_exit();
        }
    }
    if (catch) {
        //register the seg fault handler that 
        signal(SIGSEGV, catch_segfault);
    }
    if (seg) {
        //cause a segfault
        cause_seg_fault();
    }
    
    if (infile) {
        input_fd = open(infile, O_RDONLY);
        if(input_fd >= 0) {
            close(0);
            dup(input_fd);
            close(input_fd);
        }
        else {
            fprintf(stderr, "Error opening file: error = %d, message =  %s \n", errno, strerror(errno));
            exit(2);
        }
    }

    if (outfile) {
        output_fd = creat(outfile, 0666);
        if (output_fd >= 0) {
            close(1);
            dup(output_fd);
            close(output_fd);
        }
        else {
            fprintf(stderr, "Error creating file: error = %d, message =  %s \n", errno, strerror(errno));
            exit(3);
        }
    }

    char buf[128];
    int s = 0;
    while( (s = read(0, buf, 1) ) > 0) {
       int write_count =  write(1, buf, s);
       if(write_count != s) {
           fprintf(stderr, "Error: number of bytes read is not equal to bytes written. %d != %d", s, write_count);
           exit(3); //from piazza post 53
       }
    }
    exit(0);
}
