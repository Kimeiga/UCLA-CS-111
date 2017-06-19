// NAME: Rohan Varma
// EMAIL: rvarm1@ucla.
// ID: 111111111
//lab2_list.c

#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <signal.h>
#include "SortedList.h"
//pointers to the list and (unitialized) elements of it
SortedList_t* list;
SortedListElement_t *elements;
//locking mechanisms
pthread_mutex_t mutex_lock = PTHREAD_MUTEX_INITIALIZER;
int spin_lock = 0;
void* (*list_func)(void *); //all list functions are of this type
//arg vars
int num_threads = 1;
int iters = 1;
int opt_yield = 0;
//an enum for our lock types
typedef enum locks {
  none, spin, mutex
} lock_type;
lock_type lock_in_use = none;
//general error handling
void print_ops_and_exit(){
  fprintf(stderr, "Usage: ./lab2_list [--threads=t] [--iterations=i] [--yield={dli}] [--sync={m,s}] \n");
  exit(EXIT_FAILURE);
}

void err_exit(const char *msg, int num) {
  fprintf(stderr, "%s: error = %d, message = %s", msg, num, strerror(num));
  exit(EXIT_FAILURE);
}
//list lookups, inserts, deletes
void* list_regular(void *arg){
  //printf("running no locking func \n");
  SortedListElement_t *begin = arg;
  int i;
  for(i = 0; i < iters; i++){
    SortedList_insert(list, begin + i);
  }
  int len = SortedList_length(list);
  if(len == -1){
    fprintf(stderr, "error: SortedList_length indicated that list was corrupted\n");
    exit(2);
  }
  for(i = 0; i < iters; i++){
    SortedListElement_t* find = SortedList_lookup(list, begin[i].key);
    if(find == NULL){
      fprintf(stderr, "error: SortedList_lookup failed when it should've succeeded. \n");
      exit(2);
    }
    int x = SortedList_delete(find);
    if(x != 0){
      fprintf(stderr, "error: delete() failed \n");
      exit(2);
    }
  }
  return NULL;
}
//spin lock around list manuplation
void* list_spin(void *arg){
  //printf("running spin locking func \n");
  SortedListElement_t *begin = arg;
  int i;
  for(i = 0; i < iters; i++){
    while(__sync_lock_test_and_set(&spin_lock, 1) == 1) ; //spin
    SortedList_insert(list, begin + i);
    __sync_lock_release(&spin_lock);
  }
  while(__sync_lock_test_and_set(&spin_lock, 1) == 1) ; //spin
  int len = SortedList_length(list);
  __sync_lock_release(&spin_lock);
  if(len == -1){
    fprintf(stderr, "error: SortedList_length indicated that list was corrupted\n");
    exit(2);
  }
  for(i = 0; i < iters; i++){
    while(__sync_lock_test_and_set(&spin_lock, 1) == 1) ; //spin
    SortedListElement_t* find = SortedList_lookup(list, begin[i].key);
    if(find == NULL){
      fprintf(stderr,  "erroor:SortedList_lookup failed when it should've succeeded. \n");
      exit(2);
    }
    int x = SortedList_delete(find);
    if(x != 0){
      fprintf(stderr, "error: delete() failed \n");
      exit(2);
    }
    __sync_lock_release(&spin_lock);
  }
  return NULL;
}
//list manuplation with mutex lock
void* list_mutex(void *arg){
  //printf("running mutex locking func \n");
  SortedListElement_t *begin = arg;
  int i;
  for(i = 0; i < iters; i++){
    pthread_mutex_lock(&mutex_lock);
    SortedList_insert(list, begin + i);
    pthread_mutex_unlock(&mutex_lock);
  }
  pthread_mutex_lock(&mutex_lock);
  int len = SortedList_length(list);
  pthread_mutex_unlock(&mutex_lock);
  //  printf("length passed \n");
  if(len == -1){
    fprintf(stderr, "SortedList_length indicated that list was corrupted\n");
    exit(2);
  }
  for(i = 0; i < iters; i++){
    pthread_mutex_lock(&mutex_lock);
    SortedListElement_t* find = SortedList_lookup(list, begin[i].key);
    if(find == NULL){
      fprintf(stderr, "SortedList_lookup failed when it should've succeeded. \n");
      exit(2);
    }
    int x = SortedList_delete(find);
    //    printf("delete passed \n");
    if(x != 0){
      fprintf(stderr, "delete() failed \n");
      exit(2);
    }
    pthread_mutex_unlock(&mutex_lock);
  }
  return NULL;
}


void handler(int num) {
  if (num == SIGSEGV) {
    fprintf(stderr, "Seg fault caught: error = %d, message =  %s \n", num, strerror(num));
    exit(2);
  }
}

int main(int argc, char ** argv){
  int opt = 0;
  static struct option long_options[] = {
    {"threads", required_argument, 0, 't'},
    {"iterations", required_argument, 0, 'i'},
    {"yield", required_argument, 0, 'y'},
    {"sync", required_argument, 0, 's'}
  };
  while((opt = getopt_long(argc, argv, "t:s:", long_options, NULL)) != -1){
    int i;
    switch(opt){
    case 't':
      num_threads = atoi(optarg);
      break;
    case 'i':
      iters = atoi(optarg);
      break;
    case 'y':
      for(i=0; i < strlen(optarg); i++){
	if(optarg[i] == 'i')
	  opt_yield |= INSERT_YIELD;
	else if(optarg[i] == 'd')
	  opt_yield |= DELETE_YIELD;
	else if(optarg[i] == 'l')
	  opt_yield |= LOOKUP_YIELD;
	else
	  print_ops_and_exit(); //print usage and exit
      }
      break;
    case 's':
      if(strcmp("m", optarg) == 0) lock_in_use = mutex;
      else if(strcmp("s", optarg) == 0) lock_in_use = spin;
      else print_ops_and_exit();
      break;
    default:
      print_ops_and_exit();
    }
  }
  if(signal(SIGSEGV, handler) < 0) err_exit("signal failed", errno);
  //printf("args: \n");
  //printf("threads: %d, iters: %lld, sync: %s \n", num_threads, iters, lock_in_use == none ? "none": lock_in_use == spin ? "spin": "mutex");
  // if(opt_yield & INSERT_YIELD) printf("insert yield specified \n");
  // if(opt_yield & DELETE_YIELD) printf("delete yield specified \n");
  // if(opt_yield & LOOKUP_YIELD) printf("lookup yield specified \n");
  list_func = lock_in_use == none ? list_regular : lock_in_use == spin ? list_spin : list_mutex;
  //init empty list
  list = malloc(sizeof(SortedList_t));
  list->key = NULL;
  list->next = list->prev = list;
  //printf("list len: %d \n", SortedList_length(list));
  //assert(SortedList_length(list) == 0);
  long long runs = num_threads * iters;
  //init list elements
  SortedListElement_t *elements = malloc(runs * sizeof(SortedListElement_t));
  //randkeys
  srand(time(NULL));
  int i;
  for(i=0;i<runs;i++){
    int len = rand() % 15 + 1; //keys btwn 1 and 15
    int from_a_offset = rand() % 26;
    char *key = malloc((len+1) * sizeof(char)); //for the null byte
    int j;
    for(j=0;j<len;j++){
      key[j] = 'a' + from_a_offset;
      from_a_offset = rand() % 26;
    }
    key[len] = '\0'; //terminator
    elements[i].key = (const char *) key;
  }
  struct timespec start, end;
  pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start) < 0) err_exit("clock_gettime error", errno);
  for(i=0;i<num_threads;i++){
    int x = pthread_create(threads + i, NULL, list_func, elements + i*iters);
    if(x) err_exit("pthread_create failed", errno);
  }
  for(i=0;i<num_threads;i++){
    int x = pthread_join(threads[i], NULL);
    if(x) err_exit("pthread_join failed", errno);
  }
  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end) < 0) err_exit("clock_gettime error", errno);
  long long nanoseconds = (end.tv_sec - start.tv_sec) * 1000000000L + (end.tv_nsec - start.tv_nsec);
  if(SortedList_length(list) != 0){
    fprintf(stderr, "corrupted list: list length was not 0 at the end");
    exit(2);
  }
  free(list);
  free(threads);
  free(elements);
  int num_lists = 1;
  long long num_ops = num_threads * iters * 3;
  long long avg = nanoseconds/num_ops;
  char *yield_str;
  if(opt_yield == 0) yield_str = "none";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "idl";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & DELETE_YIELD)) yield_str = "id";
  else if((opt_yield & INSERT_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "il";
  else if(opt_yield & INSERT_YIELD) yield_str = "i";
  else if((opt_yield & DELETE_YIELD) && (opt_yield & LOOKUP_YIELD)) yield_str = "dl";
  else if(opt_yield & DELETE_YIELD) yield_str = "d";
  else yield_str = "l";
  printf("list-%s-%s,%d,%lld,%d,%lld,%lld,%lld\n", yield_str,
	 lock_in_use == none ? "none" : lock_in_use == mutex ? "m" : "s",
	 num_threads, iters, num_lists, num_ops, nanoseconds, avg);
  exit(EXIT_SUCCESS);
}
