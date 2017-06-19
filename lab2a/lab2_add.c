
// NAME: Rohan Varma
// EMAIL: rvarm1@ucla.
// ID: 111111111
//lab2_add.c 

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>

//a function pointer to the various add functions with different locking
void (*add_func_to_use)(long long *, long long);
//yield and iteration variables
int opt_yield = 0;
long long iterations = 1;
//an enum to see which lock mechanism we are using
typedef enum locks {
  none, mutex, spin, compare_swap
} lock_type;
lock_type lock_in_use = none; //none by default

//locks
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER; 
int spin_lock = 0;

//general error handling functions
void print_ops_and_exit(){
  fprintf(stderr, "Usage: ./lab2_add [--threads=num_threads] [--iterations=num_iterations] [--yield] [--sync={m,s,c}]\n");
  exit(EXIT_FAILURE);
}

void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, num, strerror(num));
  exit(EXIT_FAILURE);
}

//the (given) add function
void add(long long *pointer, long long value){
  long long sum = *pointer + value;
  if(opt_yield)
    sched_yield();
  *pointer = sum;
}

//adds, by adding critical section
void mutex_add(long long *pointer, long long value){
  int x = pthread_mutex_lock(&mutex_lock);
  //assert(x==0);
  //lock acquired
  add(pointer, value);
  x = pthread_mutex_unlock(&mutex_lock);
  //assert(x==0);
}

//adds, with spin lock
static void spin_add(long long *pointer, long long value)
{
  while(__sync_lock_test_and_set(&spin_lock, 1) == 1) ; //spin
  //        printf("acquired lock \n");
  add(pointer, value);
  __sync_lock_release(&spin_lock);
  //        printf("released lock \n");
}

//adds, with compare and swap instruction
void atomic_sync_add(long long *pointer, long long value){
  long long expected, sum;
  do{
    expected = *pointer;
    sum = expected + value;
    if(opt_yield)
      sched_yield();
  } while(expected != __sync_val_compare_and_swap(pointer, expected, sum));
}

//add 1 and -1 iterations times
void* thread_func(void *counter){
  int i;
  for(i=0;i<iterations;i++){
    add_func_to_use((long long *) counter, 1);
  }
  for(i=0;i<iterations;i++){
    add_func_to_use((long long *) counter, -1);
  }
  return NULL;
}

int main(int argc, char **argv){
  //argument variables
  int opt, num_threads=1;
  long long counter=0;
  static struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"yield", no_argument, 0, 'y'},
    {"sync", required_argument, 0, 's'},
    {"debug", no_argument, 0, 'd'},
    {"iterations", required_argument, 0, 'i'},
  };
  while((opt = getopt_long(argc, argv, "t:i:", long_options, NULL)) != -1){
    switch(opt){
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'i':
      iterations = strtoll(optarg, NULL, 10);
      break;
    case 'y':
      opt_yield = 1;
      break;
    case 's':
      if(strlen(optarg) == 1 && strcmp("m", optarg) == 0) lock_in_use = mutex;
      else if(strlen(optarg) == 1 && strcmp("s", optarg) == 0) lock_in_use = spin;
      else if(strlen(optarg) == 1 && strcmp("c", optarg) == 0) lock_in_use = compare_swap;
      else print_ops_and_exit(); //received invalid lock request
      break;
    default:
      print_ops_and_exit();
    }
  }
  //figure out which add function to use, corresponding to the lock mechanism
  add_func_to_use = lock_in_use == none ? add : lock_in_use == spin ? spin_add : lock_in_use == mutex ? mutex_add : atomic_sync_add;
  //initialize time structs and threads
  struct timespec start, end;
  pthread_t *threads;
  threads = malloc(num_threads*sizeof(pthread_t));
  int i;
  //start keeping time and create & join threads
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) < 0) err_exit("clock_gettime error", errno);
  for(i=0;i<num_threads;i++){
    if(pthread_create(threads + i, NULL, thread_func, &counter) != 0) err_exit("thread creation error", errno);
  }
  for(i=0;i<num_threads;i++){
    if(pthread_join(threads[i], NULL) != 0) err_exit("thread join error", errno);
  }
  //note the end time fo the run
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) < 0) err_exit("clock_gettime error", errno);
  //compute nanoseconds elapsed 1000000000
  long long nanoseconds = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
   free(threads);
  //prints the csv
  printf("add%s-%s,%d,%lld,%lld,%lld,%lld,%lld\n", opt_yield == 0 ? "":"-yield", 
	 lock_in_use == none ? "none" : lock_in_use == mutex ? "m": lock_in_use == spin ? "s" : "c", 
	 num_threads, iterations, num_threads * iterations * 2, nanoseconds, 
	 nanoseconds/(num_threads * iterations * 2), counter); 
  exit(EXIT_SUCCESS);
}
