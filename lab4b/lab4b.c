// NAME: Rohan Varma
// EMAIL: rvarm1@ucla.edu
// ID: 111111111
//lab4b.c
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <mraa.h>
#include <aio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include <limits.h>
//arg vars
int first_temp_read = 0; 
long interval = 1;
int scale = 'F';
char *log_file;
int log_flag = 0;
int debug = 0; 
FILE *logger;
int generating;
//sensor & button
mraa_aio_context sensor;
mraa_gpio_context button;
//general error handling
void print_ops_and_exit(){
	fprintf(stderr, "Usage: ./lab4b [--period=p] [--scale={C,F}] [--log=filename]");
	exit(EXIT_FAILURE);
}
void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, num, strerror(num));
  exit(EXIT_FAILURE);
}
//temperature converts
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
	if(!convert) err_exit("localtime", errno);
	strftime(real_time, 9, "%H:%M:%S", convert);
}
//command processing
void handle_off(){
	char time[10];
	get_time(time);
	if(log_flag){
		fprintf(logger, "%s\n", "OFF");
		fprintf(logger, "%s %s\n", time, "SHUTDOWN");
	}
	//fprintf(stdout, "%s %s\n", time, "SHUTDOWN"); from piazza 510: don't have to print shutdown to stdout
	//if(log_flag) fflush(logger);
	exit(EXIT_SUCCESS);
}
void handle_stop_start(int stop_or_start){
	assert(stop_or_start == 0 || stop_or_start == 1);
	//if 0 stop else start
	generating = stop_or_start;
	if(log_flag){
		fprintf(logger, "%s\n", stop_or_start == 0 ? "STOP" : "START");
		//fflush(logger);
	}
}
void handle_change_scale(char c_or_f){
	assert(c_or_f == 'C' || c_or_f == 'F');
	scale = c_or_f;
	if(log_flag){
		fprintf(logger, "%s\n", c_or_f == 'F' ? "SCALE=F" : "SCALE=C");
	//	fflush(logger);
	}
}
void handle_period_change(char *buf){
//	code adapted from http://man7.org/linux/man-pages/man3/strtol.3.html
	long val;
	char *str = buf + strlen("PERIOD=");
	char *end;
	errno = 0;
	val = strtol(str, &end, 10);
	//since this won't be checked (from a Piazza post) we're just gonna terminate if PERIOD=x is bad
	if( (errno = ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)){
		err_exit("strtol: invalid number", errno);
	}
	else if(end == str){
		err_exit("strtol: no numbers found", errno);
	}
	else if(val < 0){
		err_exit("negative value", errno);
	}
	else{
		interval = val;
		if(log_flag){
			fprintf(logger, "%s%lu\n", "PERIOD=", val);
		//	fflush(logger);
		}
	}
}
//button polling thread
void * btn_func(void * arg){
//	if(debug) printf("in io func thread \n");
	char buf[512];
	while(1){
		//query btn 
		int btn = mraa_gpio_read(button);
		if(btn){
			char time[10];
			get_time(time);
			if(log_flag) fprintf(logger, "%s %s\n", time, "SHUTDOWN");
			//fprintf(stdout, "%s %s \n", time, "SHUTDOWN"); from piazza 510: don't have to print shutdown to stdout
			//if(log_flag) fflush(logger);
			exit(EXIT_SUCCESS);
		}
	}
	return NULL;
}
//temperature reporting thread
void * temp_func(void * arg){
	while (1) {
    	int ctemp = mraa_aio_read(sensor);
    	float temp = scale == 'F' ? c_to_f(temp_convert(ctemp)) : temp_convert(ctemp); 
    	char time[10];
    	get_time(time);
    	if(generating && log_flag) fprintf(logger, "%s %.1f\n", time, temp); //log temp only if generating and log flag
    	if (generating) fprintf(stdout, "%s %.1f \n", time, temp); //write temp only if generating
    	//if(generating && log_flag) fflush(logger);
    	if(!first_temp_read) first_temp_read=1; //so that the commands thread can begin processing
    	sleep(interval);
    }
    return NULL;
}
//command processing thread
void * process_commands(void *arg){
	char buf[512];
	while(1) {
		if(!first_temp_read) continue; //just spin for the first temperature read because it will happen quickly
		int x = read(STDIN_FILENO, buf, 512);
		if(x < 0) err_exit("read failed \n", errno);
		else if (x > 0){
			int i; int start = 0; //for multi command processing
			for(i = 0; i < x; i++){
				if(buf[i] == '\n'){ //from piazza: commands are logged (if logging is on) but never output to stdout.
					buf[i] = 0 ; //set null byte to where \n is
					if(strcmp(buf + start, "OFF") == 0) handle_off();
					else if(strcmp(buf + start, "STOP") == 0) handle_stop_start(0);
					else if(strcmp(buf + start, "START") == 0) handle_stop_start(1);
					else if (strcmp(buf + start, "SCALE=C") == 0) handle_change_scale('C');
					else if(strcmp(buf + start, "SCALE=F") == 0) handle_change_scale('F');
					else if(strncmp(buf + start, "PERIOD=", strlen("PERIOD=")) == 0) handle_period_change(buf + start);
					else {
					//command is invalid. by @448: Log the error message in the file and exit (without any output to stdout)
						if(log_flag){
							char *k = buf + start;
							fprintf(logger, "%s %s\n", "Invalid command:", k);
						}
						exit(EXIT_FAILURE);
					}
					start = i + 1; //begin command processing (if we have more) after the \n 
				}
			}
		}
	}
	return NULL;
}
 
int main(int argc, char **argv){
	int opt; 
	generating = 1; //we are generating reports by default. STOP turns this to 0 (and START back to 1)
	static struct option long_options[] = {
		{"period", required_argument, 0, 'p'},
		{"scale", required_argument, 0, 's'},
		{"log", required_argument, 0, 'l'},
		{"debug", no_argument, 0, 'd'}, 
		{0, 0, 0, 0}
	};
	while((opt = getopt_long(argc, argv, "p:d", long_options, NULL)) != -1) {
		switch(opt) {
			case 'p':
				; //idk but this shut the compiler up. 
				char *str = optarg;
				char *end; 
				errno = 0; 
				long val;
				val = strtol(str, &end, 10);
				if((errno = ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)){
					fprintf(stderr, "strtol: invalid num \n");
					print_ops_and_exit();
				}
				else if (end == str){
					fprintf(stderr, "strtol: no nums found\n");
					print_ops_and_exit();
				}
				else if (val < 0){
					fprintf(stderr, "cannot have neg interval \n");
					print_ops_and_exit();
				}
				interval = val;
				break;
			case 's':
				if(strcmp("C", optarg) == 0) scale = 'C';
				else if(strcmp("F", optarg) == 0) scale = 'F';
				else{
					fprintf(stderr, "error: scale argument must be either F or C (caps matter) \n");
					print_ops_and_exit();
				}
				break;
			case 'l':
				log_file = optarg;
				log_flag = 1;
				break;
			case 'd':
				debug = 1;
				break;
			default:
				print_ops_and_exit();
		}
	}
	//init
	if(log_flag){
		logger = fopen(log_file, "a");
		if(!logger) err_exit("fopen", errno);
	}
    sensor = mraa_aio_init(0);
    if(!sensor) err_exit("mraa_aio_init", errno);
    button = mraa_gpio_init(3);
    if(!button) err_exit("mraa_gpio_init", errno);
    //3 threads
    pthread_t *threads = malloc(3 * sizeof(pthread_t));
    if(!threads) err_exit("malloc", errno);
    int i;
    for(i = 0; i < 3; i++){
    	int x = pthread_create(threads + i, NULL, i == 0 ? temp_func : i == 1 ? btn_func : process_commands, NULL);
    	if(x) err_exit("pthread create", errno);
    }
    for(i = 0 ; i < 3; i++){
    	int x=pthread_join(threads[i], NULL);
    	if(x) err_exit("pthread_join", errno);
    }
    //cleanup
    free(threads);
    if(log_flag){
    	int x=fclose(logger);
    	if(x) err_exit("fclose", errno);
    }
    mraa_aio_close(sensor);
    mraa_gpio_close(button);
}
