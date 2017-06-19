/*
Name: Rohan Varma
ID: 111111111
email: rvarm1@ucla.edu
 */
#include <termios.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/poll.h>
#include <signal.h>

const int SHELL_ON = 1;
const int SHELL_OFF = 0;
char buf[256];
int shell = 0;
int debug = 0;
pid_t cpid;
struct termios saved_state;
int to_child_pipe[2]; //read terminal -> write to shell pipe
int from_child_pipe[2]; //write terminal <-- read shell pipe


/* -----Utility functions------ */
void print_ops_and_exit() {
    fprintf(stderr, "Usage: lab1a [--shell] \n");
    exit(EXIT_FAILURE);
}

void err_exit(const char *msg, int num) {
    fprintf(stderr, "%s: error = %d, message = %s", msg, errno, strerror(errno));
    exit(EXIT_FAILURE);
}

int safe_read(int fd, char *buf, int s) {
    ssize_t nbytes = read(fd, buf, s);
    if (nbytes < 0) err_exit("read error", errno);
    return nbytes;
}

int safe_write(int fd, char *buf, int s) {
    ssize_t nbytes = write(fd, buf, s);
    if (nbytes < 0) err_exit("write error", errno);
    return nbytes;
}

int safe_close(int fd) {
    int e = close(fd);
    if (e < 0) err_exit("close error", errno);
    //if (e < 0 && debug) fprintf(stderr, "CLOSE ERROR, CHECK VALUES \n");
    return e;
}

int safe_dup2(int fd1, int fd2) {
    int e = dup2(fd1, fd2);
    if (e < 0) err_exit("dup2 error", errno);
    return e;
}

/* ----- Debug functions ------ */

void print_flag_values(const struct termios *state) {
        int iflag = state->c_iflag;
        int oflag = state->c_oflag;
        int lflag = state->c_lflag;
        printf("%d \n", iflag);
        printf("%d \n", oflag);
        printf("%d \n", lflag);
}

int cmp_flag_values(const struct termios *s1, const struct termios *s2) {
    int i1 = s1->c_iflag;
    int i2 = s2->c_iflag;
    int o1 = s1->c_oflag;
    int o2 = s2->c_oflag;
    int l1 = s1->c_lflag;
    int l2 = s2->c_lflag;
    return (i1 == i2 && o1 == o2 && l1 == l2) ? 1:0;
}

/* ----- Terminal struct state functions ------ */

void save_normal_settings() {
    int s = tcgetattr(STDIN_FILENO, &saved_state);
    if (s < 0) {
        err_exit("Error getting terminal params", errno);
     }
    if (debug) {
        struct termios tst;
        tcgetattr(STDIN_FILENO, &tst);
        int cmp = cmp_flag_values( &tst, &saved_state);
        if (cmp == 0) {
            printf("warning: normal terminal setting may have not been restored \n");
        }
    }
 }

void shell_wait_and_clean() {
    int stat_loc;
    pid_t waited = waitpid(cpid, &stat_loc, 0);
    if (waited == -1) err_exit("waitpid failed", errno);
    if (debug) printf("done waiting \n");
    fprintf(stderr, "SHELL EXIT SIGNAL=%d STATUS=%d\n", WTERMSIG(stat_loc), WEXITSTATUS(stat_loc));
}

void restore_normal_settings() {
    if (debug) printf("restoring normal terminal settings upon exit \n");
    
    int s = tcsetattr(STDIN_FILENO, TCSANOW,  &saved_state);
    if (s < 0) {
        err_exit("error restoring original terminal params", errno);
    }
    if (shell) {
        if (debug) printf("about to wait \n");
        shell_wait_and_clean();
    }
    if (debug) {
        struct termios tst;
        tcgetattr(STDIN_FILENO, &tst);
        int cmp = cmp_flag_values(&tst, &saved_state);
        if (cmp == 0) {
            printf("warning: normal terminal settings may have not been restored \n");
        }
    }
}

/* --- Signal Handler --- */

void handler(int sig) {
    if (debug) {
        printf("IN SIGNAL HANDLER \n");
    }
    if (sig == SIGPIPE) {
        if (debug) {
            printf("received sigpipe \n");
        }
        exit(EXIT_SUCCESS);
    }
    // if (sig == SIGINT) {
    //     if (debug) printf("recieved sigint, calling kill \n" );
    //     kill(cpid, SIGINT);

    // }
}

/* ----- Read & Write functions ------ */

void do_write(char *buf, int ofd, int nbytes, int shell) {
    int i;
    for(i = 0; i < nbytes; i++) {
        switch (*(buf+i)) {
            case 0x04:
                if (shell) {
                    int e = close(to_child_pipe[1]); //close the pipe to the shel
                    if (debug) {
                        if (e < 0) printf("VALUE OF SAFE CLOSE IN ^D CHECK IS BAD: %d \n", e);
                        else printf("VALUE OF SAFE CLOSE IN ^D CHECK IS OKAY \n");
                        printf("closed/sent EOF \n ");
                    }
                }
                else exit(EXIT_SUCCESS);
                break;
            case 0x03:
                if (debug && shell) {
                    printf("about to send kill to cpid \n");
                }
                if (shell) kill(cpid, SIGINT);
                break;
            case '\r':
            case '\n':
                if (ofd == STDOUT_FILENO) {
                    char tmp[2]; tmp[0] = '\r'; tmp[1] = '\n'; 
                    safe_write(ofd, tmp, 2);
                }
                else {
                    if (debug) {
                        if (!shell) printf("%d\n", shell);
                    }
                    char tmp[0]; tmp[0] = '\n';
                    safe_write(ofd, tmp, 1);
                }
                break;
            default:
                safe_write(ofd, buf + i, 1);

        }
    }
}

 /* --- Main -- */

int main(int argc, char **argv) {
    int opt = 0;
    static struct option long_options[] = {
        {"shell", no_argument, 0, 's'},
        {"debug", no_argument, 0, 'd'},
        {0, 0, 0, 0}
    };
    while((opt = getopt_long(argc, argv, "s:d", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            shell = 1;
            break;
        case 'd':
            debug = 1;
            break;
        default:
            print_ops_and_exit();
        }
    }
    if(debug) {
        if (shell) printf("shell arg specified \n"); 
    }

    //first save normal terminal settings
    save_normal_settings(); //saved state now has the normal terminal settings
    atexit(restore_normal_settings); //restore normal settings upon exit
    struct termios non_canonical_input_mode;
    // put current state of terminal into new termios struct, then manually change it
    tcgetattr(STDIN_FILENO, &non_canonical_input_mode);
    non_canonical_input_mode.c_iflag = ISTRIP; //only lower 7 bits
    non_canonical_input_mode.c_oflag = 0; // no processing
    non_canonical_input_mode.c_lflag = 0; // no processing
    int s = tcsetattr(STDIN_FILENO, TCSANOW, &non_canonical_input_mode);
    if (s < 0) {
        err_exit("error setting terminal", errno);
    }
    if (debug) {
        struct termios tst;
        tcgetattr(STDIN_FILENO, &tst);
        int cmp = cmp_flag_values(&tst, &non_canonical_input_mode);
        int cmp2 = cmp_flag_values(&tst, &saved_state);
        if(cmp == 0 || cmp2 == 1){
            printf("something got messed up \n");
        }
    }

    if (shell) {
        if (debug) printf("in shell option \n");
        signal(SIGPIPE, handler);
      //  signal(SIGINT, handler);
        if (debug) printf(" registered signal handlers for shell\n");
        if(pipe(to_child_pipe) < 0) {
            err_exit("pipe failed", errno);
        }
        if(pipe(from_child_pipe) < 0){
            err_exit("pipe failed", errno);
        }
        if (debug) printf("finished setting up pipes \n");
        cpid = fork();
        if (cpid < 0) {
            err_exit("fork failed", errno);
        }
        else if (cpid == 0){
            if (debug) printf("in child process \n");
            //we don't need these pipes
            int x;
            x = safe_close(to_child_pipe[1]);
            if(x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            x = safe_close(from_child_pipe[0]);
            if (x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            safe_dup2(to_child_pipe[0], 0);
            safe_dup2(from_child_pipe[1], 1);
            safe_dup2(from_child_pipe[1], 2);
            //dup'd them so we don't need the pipes
            x = safe_close(to_child_pipe[0]);
            if (x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            x = safe_close(from_child_pipe[1]);
            if (x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            int e = execvp("/bin/bash", NULL);
            if (e < 0) {
                err_exit("execvp failed", errno);
            }
        }
        else {
            if (debug) printf("in parent process \n");
            int x;
            x = safe_close(to_child_pipe[0]);
            if (x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            x = safe_close(from_child_pipe[1]);
            if (x < 0 && debug) {
                printf("VALUE OF RECENT SAFE CLOSE: %d \n", x);
            }
            int pipe_input_fd = from_child_pipe[0];
            int pipe_output_fd = to_child_pipe[1];
            struct pollfd foo[2];
            // first pollfd describes stdin
            foo[0].fd = STDIN_FILENO; //reads from stdin
            foo[0].events = POLLIN;
            //next pollfd describes pipe that returns out frm shell
            foo[1].fd = from_child_pipe[0]; //reads from shell
            foo[1].events = POLLIN | POLLHUP | POLLERR;
            while(1) {
                int r = poll(foo, 2, 0);
                if (r < 0) {
                    err_exit("poll failed", errno);
                }
                else {
                    if (r == 0) continue;
                    if(foo[0].revents & POLLIN) {
                        //read from STDIN
                        char from_stdin[256];
                        int bytes_read = safe_read(STDIN_FILENO, from_stdin, 256);
                        //echo to STDOUT and shell
                        do_write(from_stdin,STDOUT_FILENO,bytes_read,SHELL_ON);
                        do_write(from_stdin, to_child_pipe[1], bytes_read, SHELL_ON);
                    }
                    //in from pipe
                    if (foo[1].revents & POLLIN) {
                        //read from shell and write to terminal (STDOUT)
                        char from_shell[256];
                        int bytes_read = safe_read(foo[1].fd, from_shell, 256);
                        do_write(from_shell, STDOUT_FILENO, bytes_read, SHELL_ON);
                    }
                    if (foo[1].revents & (POLLHUP | POLLERR)) {
                        if (debug)  fprintf(stderr, "SHELL SHUT DOWN \n");
                       int y = close(to_child_pipe[1]); //close pipe to the shell 
                       if (debug) {
                        if (y < 0) fprintf(stderr, "CLOSE IN POLLERR ERROR: %d", y);
                        else printf("VALUE OF Y IN POLLER ERROR: %d", y);
                       }
                        break; 
                    }
                }
            }

        }
    }
    else {
        if (debug) printf("in default option \n");
        ssize_t nbytes = safe_read(STDIN_FILENO, buf, 256);
        while(nbytes > 0) {
            do_write(buf, STDOUT_FILENO, nbytes, SHELL_OFF);
            nbytes = safe_read(STDIN_FILENO, buf, 1);
        }
    }
}
