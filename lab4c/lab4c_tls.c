// NAME: Rohan Varma
// EMAIL: rvarm1@ucla.edu
// ID: 111111111
//lab4c_tls.c

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <time.h>
#include <pthread.h>
#include <mraa.h>
#include <aio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/evp.h>

char * DEFAULT_ID = "111111111";
char * DEFAULT_HOSTNAME = "lever.cs.ucla.edu";
//SSL variables
SSL_CTX* ssl_context;
SSL * ssl;
int DEBUG = 0; // set to 0 to turn debugging off
char *id;
int id_arg_given = 0; 
long interval = 1;
int scale = 'F';
int generating = 1; 
FILE *logger;
int port;
//sensor & button
mraa_aio_context sensor;
char *hostname;
char *log_file;
int log_flag;
time_t last_report;

void print_ops_and_exit(){
  fprintf(stderr, "Usage: ./lab4c_tls --id=[ID] --host=[HOSTNAME] --log=[LOGFILE] portnum\n");
  exit(EXIT_FAILURE);
}

void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, num, strerror(num));
  exit(EXIT_FAILURE);
}

float c_to_f(float a){
  return a * 1.8 + 32.0;
}
float temp_convert(int a){
    float R = 100000.0 * (1023.0/((float)a)-1.0);
    int B = 4275;
    return 1.0/(log(R/100000.0)/B+1/298.15)-273.15;
}
//time
void get_time(char *real_time){
  time_t t; 
  time(&t);
  //from http://man7.org/linux/man-pages/man3/strftime.3.html
  struct tm* convert = localtime(&t);
  if(!convert) {
  	fprintf(stderr, "error: localtime()\n");
  	exit(2);
  }
  strftime(real_time, 9, "%H:%M:%S", convert);
}

void handle_off(int sockfd){
  char time_buf[10];
  get_time(time_buf);
  char shut_buf[20];
  sprintf(shut_buf, "%s SHUTDOWN\n", time_buf);
  if(SSL_write(ssl, shut_buf, strlen(shut_buf)) < 0) {
  	fprintf(stderr, "error SSL writing to server\n");
  	exit(2);
  }
  if(log_flag){
    fprintf(logger, "%s\n", "OFF");
    fprintf(logger, "%s %s\n", time_buf, "SHUTDOWN");
    fflush(logger);
  }
  exit(EXIT_SUCCESS);
}
void handle_change_scale(char c_or_f, int sockfd){
  assert(c_or_f == 'C' || c_or_f == 'F');
  scale = c_or_f;
  if(log_flag){
    fprintf(logger, "%s\n", c_or_f == 'F' ? "SCALE=F" : "SCALE=C");
    fflush(logger);
  }
}
void handle_stop_start(int stop_or_start, int sockfd){
  assert(stop_or_start == 0 || stop_or_start == 1);
  //if 0 stop else start
  generating = stop_or_start;
  if(log_flag){
    fprintf(logger, "%s\n", stop_or_start == 0 ? "STOP" : "START");
    fflush(logger);
  }
}

void handle_period_change(char *buf, int sockfd){
//  code adapted from http://man7.org/linux/man-pages/man3/strtol.3.html
  long val;
  char *str = buf + strlen("PERIOD=");
  char *end;
  errno = 0;
  val = strtol(str, &end, 10);
  //since this won't be checked (from a Piazza post) we're just gonna terminate if PERIOD=x is bad
  if( (errno = ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)){
    fprintf(stderr, "strtol: invalid number\n");
    exit(2);
  }
  else if(end == str){
    fprintf(stderr, "strtol: no nums found\n");
    exit(2);
  }
  else if(val < 0){
    fprintf(stderr, "error: period should not be negative\n");
    exit(2);
  }
  else{
    interval = val;
    if(log_flag){
      fprintf(logger, "%s%lu\n", "PERIOD=", val);
      fflush(logger);
    }
  }
}

int main(int argc, char **argv){
  //defaults
  id = DEFAULT_ID;
  hostname = DEFAULT_HOSTNAME;
  log_flag = 0;
  if (argc == 1){
  	fprintf(stderr, "an arg for the port num is required\n");
  	print_ops_and_exit();
  }
  int opt;
  static struct option long_options[] = {
    {"id", required_argument, 0, 'i'},
    {"host", required_argument, 0, 'h'},
    {"log", required_argument, 0, 'l'},
    {0, 0, 0, 0}
  };
  while((opt = getopt_long(argc, argv, "i:h:l:", long_options, NULL)) != -1) {
    switch(opt) {
      case 'i':
        if(strlen(optarg) != 9) {
          fprintf(stderr, "Using default ID: %s since input was not 9 digit\n", id);
        }
        else {
        	id_arg_given = 1;
          id = (char *) malloc(sizeof(char) * 10);
          strcpy(id, optarg);
          if(DEBUG) {
            printf("set id to:%s\n", id);
          }
        }
        break;
      case 'h':
        hostname = optarg;
        break;
      case 'l':
        log_file = optarg; 
        log_flag = 1;
        break;
      default:
        print_ops_and_exit();
    }
  }
  //init log
  if(log_flag) {
    logger = fopen(log_file, "a");
    if(!logger) {
    	fprintf(stderr, "fopen() on log\n");
    	exit(2);
    }
  }
  //init sensor
  sensor = mraa_aio_init(0);
  if(!sensor) {
  	fprintf(stderr, "mraa_aio_init\n");
  	exit(2);
  }
  //init port
  port = atoi(argv[optind]);
  //TODO wrong
  if(port == 0) {
    fprintf(stderr, "port num error\n");
    exit(1);
  }
  if(DEBUG) {
    //sanity check arguments
    printf("port: %d\n", port);
    if(log_flag){
      printf("log file: %s\n", log_file);
    }
    if(hostname){
      printf("hostname: %s\n", hostname);
    }
    if(id) {
      printf("id: %s\n", id);
    }
  }

  //socket
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    fprintf(stderr, "error making socket\n");
    exit(2);
  }
  //server
  struct hostent *server = gethostbyname(hostname);
  if(server == NULL){
    fprintf(stderr, "no such server\n");
    exit(1);
  }
  //connection
  struct sockaddr_in addr;
  addr.sin_port = htons(port);
  addr.sin_family = AF_INET;
  memcpy( (char *) &addr.sin_addr.s_addr, (char *) server->h_addr, server->h_length);
  if(connect(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0){
    fprintf(stderr, "error connecting to server\n");
    exit(2);
  }

  //initialize ssl lib
  if (SSL_library_init() < 0) {
  	fprintf(stderr, "error initializing openssl lib\n");
  	exit(2);
  }
  OpenSSL_add_all_algorithms();
  //create ssl context
  ssl_context = SSL_CTX_new(TLSv1_client_method());
  if(ssl_context == NULL) {
  	fprintf(stderr, "error creating ssl context\n");
  	exit(2);
  }
  //set file descriptor & connect
  ssl = SSL_new(ssl_context);
  if(SSL_set_fd(ssl, sockfd) == 0) {
  	fprintf(stderr, "error setting sockfd as SLL's fd\n");
  	exit(2);
  }
  if(SSL_connect(ssl) != 1) {
  	fprintf(stderr, "error establishing\n");
  	exit(2);
  }
  //report id
  char id_report[15];
  sprintf(id_report, "ID=%s\n", id);
  if(SSL_write(ssl, id_report, strlen(id_report)) < 0) {
  	fprintf(stderr, "could not write id\n");
  	exit(2);
  }

  if(log_flag) {
    fprintf(logger, "ID=%s\n", id);
    fflush(logger);
  }

  //server polling struct
  struct pollfd foo[1];
  foo[0].fd = sockfd;
  foo[0].events = POLLIN;
  last_report = 0;

  char buf[512];
  //repeatedly poll
  while(1) {
    int ret = poll(foo, 1, 0);
    if(ret == -1) {
      fprintf(stderr, "poll error\n");
      exit(2);
    }
    if(foo[0].revents & POLLIN) {
      int x = SSL_read(ssl, buf, 512);
      if(x < 0) err_exit("read failed\n", errno);
      else if(x > 0) {
        int i; int start = 0; //for multi command processing
        for(i = 0; i < x; i++){
          if(buf[i] == '\n'){
            buf[i] = 0 ; //set null byte to where \n is
            if(strcmp(buf + start, "OFF") == 0) handle_off(sockfd);
            else if(strcmp(buf + start, "STOP") == 0) handle_stop_start(0, sockfd);
            else if(strcmp(buf + start, "START") == 0) handle_stop_start(1, sockfd);
            else if (strcmp(buf + start, "SCALE=C") == 0) handle_change_scale('C', sockfd);
            else if(strcmp(buf + start, "SCALE=F") == 0) handle_change_scale('F', sockfd);
            else if(strncmp(buf + start, "PERIOD=", strlen("PERIOD=")) == 0) handle_period_change(buf + start, sockfd);
            else {
            //command is invalid
              if(log_flag){
                char *k = buf + start;
                fprintf(logger, "%s %s\n", "Invalid command:", k);
              }
              exit(2);
            }
            start = i + 1; //begin command processing (if we have more) after the \n 
          }
       }
      }
    } 
    time_t cur = time(0);
    if(cur - last_report >= interval && generating) {
      int ctemp = mraa_aio_read(sensor);
      float temp = scale == 'F' ? c_to_f(temp_convert(ctemp)) : temp_convert(ctemp); 
      char tbuf[10];
      last_report = time(0);
      get_time(tbuf);
      if(generating) {
        //report to server
        char tmp_buf[15];
        sprintf(tmp_buf, "%s %2.1f\n", tbuf, temp);
        if(SSL_write(ssl, tmp_buf, strlen(tmp_buf)) < 0) {
        	fprintf(stderr, "ssl write error\n"); 
        	exit(2);
        }
      }
      if(generating && log_flag) {
        fprintf(logger, "%s %2.1f\n", tbuf, temp);
        fflush(logger);
      }
    }
  }
  if(close(sockfd) < 0) {
  	fprintf(stderr, "err: close on sockfd\n");
  	exit(2);
  }
  if(id_arg_given) {
  	//that means its been mallocd
  	free(id);
  }
  SSL_shutdown(ssl);
  mraa_aio_close(sensor);
}
