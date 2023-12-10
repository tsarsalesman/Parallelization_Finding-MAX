/*
 * TO COMPILE: gcc -o pmax pmax.c -pthread
 *
 * Find the maximum value in a set of N values using M threads
 * Adapted from Dive Into Systems at Swarthmore.
 * 
 * HENRY WANDOVER, BARD COLLEGE FALL 2023
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#define MAX_THREAD 64 // Max # of threads
#define MAX_RANGE LONG_MAX // restrict the range of values generated you can set this to something small for testing for correctness (so you can more easily see the max value)

// TODO: global variables shared by all threads
static long *values; // the array of values from which max is found
static long result;
static int nthreads; // number of threads to employ
static unsigned long N;  // number of items in array
static long *max_values; // array for all max values found per thread

// TODO:if you want some shared synchronization primatives, declare
// them here (you can also init them here too).  For example:
static pthread_mutex_t result_mutex = PTHREAD_MUTEX_INITIALIZER;
// You might ignore this until we've discussed shared memory.


// TODO:if you want to pass each thread's main function more than a single
// value, you should define a new struct type with field values for
// each argument you want to pass it
struct ThreadInfo {
  int id;
  long start;
  long end;
};

// Function prototypes

/**
 * thread main routine that finds max
*/
void *thread_mainloop(void *arg);
/** 
 * inits an array to random values
 * 
 * @param values: the array to init
 * @param n: size of the array
*/
void init_values(long *values, unsigned long n);
/**
 * Prints out an array of values (only try on a small n)
 * 
 * @param values: the array to init
 * @param n: size of the array
*/
void print_values(long *values, unsigned long n);
/**
 * Prints max value w/o threading/parralelism
 * @param values: the array to init
 * @param n: size of the array
*/
void print_max(long *values, unsigned long n);
/*error handling function: prints out error message*/
int print_error(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(2);
}

/**
 * Starts clock and returns instance
 * @param tstart: time start
 * @return the tstart instance
*/
struct timeval start(struct timeval tstart) { 
  gettimeofday(&tstart, NULL); 
  return tstart;
}

/**
 * Stops clock and returns the total time
 * @param tstart: time start
 * @param tend: time end
 * @return total time (double)
 * 
*/
double stop(struct timeval tstart, struct timeval tend) {
  gettimeofday(&tend, NULL);
  double timer = tend.tv_sec - tstart.tv_sec + (tend.tv_usec - tstart.tv_usec)/1.e6;
  return timer;
}

/****************************************************************/
// TODO: implement your thread masin loop function here
void *find_max_threads(void *arg) {
  struct ThreadInfo *thread_info = (struct ThreadInfo *)arg;
  int i;
  int max = 0;

  for (i = thread_info->start; i < thread_info->end; i++) {
        if (values[i] > max) {
            max = values[i];
        }
    }
  
  // Update the shared max_values array
  max_values[thread_info->id] = max;

  // Use a mutex to protect the critical section where result is updated
  pthread_mutex_lock(&result_mutex);
  if (max > result) {
    result = max;
  }
  pthread_mutex_unlock(&result_mutex);

  return NULL;
}

/****************************************************************/
void init_values(long *values, unsigned long n) {
  unsigned long i;
  // seed the random number generator: different values based on current time
  srand(time(0));
  for(i= 0; i < n; i++) {
    values[i] = rand() % MAX_RANGE;  // let's just make these a limited range
  }
}

void print_values(long *values, unsigned long n) {
  unsigned long i;
  for(i= 0; i < n; i++) {
    printf("%6ld ", values[i]);
    if((i<(n-1)) && (((i+1)%10) == 0)) {
      printf("\n");
    }
  }
  printf("\n");
}

void print_max(long *values, unsigned long n) {
  unsigned long i;
  if (n < 1) return;
  long max = values[0];
  for(i= 1; i < n; i++) {
    if (values[i] > max)
      max = values[i];
  }
  printf("Non-parallel max value:\t%10ld\n", max);
}
/****************************************************************************
  USAGE: ./pmax N num_threads will print the maximum value of N random values.
*****************************************************************************/
int main(int argc, char **argv) {
  int i;
  srand(time(NULL));
  double timer;
  struct timeval tstart, tend;

  printf("Finds max of N values, using M threads (between 1 and %d)\n", MAX_THREAD);

  if(argc != 3) {
    printf("USAGE: %s N num_threads\n",argv[0]);
    exit(1);
  }
    
  result = 0;
  N = atol(argv[1]);
    
  nthreads = atoi(argv[2]);
  // make nthreads a sane value if insane
  if((nthreads < 1)) { nthreads = 1; }
  if((nthreads > MAX_THREAD)) { nthreads = MAX_THREAD; }
  printf("%d threads finding max of %lu values\n\n", nthreads, N);

  values = malloc(sizeof(long)*N);
  if(!values) {
    printf("mallocing up %lu longs failed\n", N);
    exit(1);
  }
  init_values(values, N);

  //print_values(values, N); // don't run for large size N if you do this
  tstart= start(tstart);
  print_max(values, N);
  timer = stop(tstart, tend);
  printf("Time is %g\n", timer);
  /*================================================================================*/
  // TODO: add your soution below

  // Initialize shared data
  max_values = (long *)malloc(sizeof(long) * nthreads);

  tstart= start(tstart);
  struct ThreadInfo *thread_info= malloc(nthreads * sizeof(struct ThreadInfo));
  pthread_t *threads= malloc(nthreads * sizeof(pthread_t)); // pointer to future thread array
  double alloc_timer= stop(tstart, tend);
  long *thread_ids= malloc(nthreads*sizeof(long));

  tstart= start(tstart);
  // creates nthreads, and calls mainloop to find max
  for (i= 0; i < nthreads; i++) {
    thread_ids[i]= i;
    thread_info[i].start= i * (N / nthreads);
    thread_info[i].end= (i + 1) * (N / nthreads);
    pthread_create(&threads[i], NULL, find_max_threads, &thread_info[i]);
  }
  double create_timer = stop(tstart, tend);


  tstart= start(tstart);
  // Join threads back together once complete; main will pause untill al threads are joined
  for (i= 0; i < nthreads; i++) {
    pthread_join(threads[i], NULL);
  }
  double join_timer = stop(tstart, tend);
  tstart= start(tstart);
  // Finds max element based on indvidual threads
  for (i= 0; i < nthreads; i++) {
    //if (max_num[i] > result) result= max_num[i];
     if (max_values[i] > result) result = max_values[i];
  }
  double findmax_timer= stop(tstart, tend);
  //printf("Time for allocation %g\n", alloc_timer);
  //printf("Time for creation of threads %g\n", create_timer);
  //printf("Time for joining threads %g\n", join_timer);
  //printf("Time for finding of max %g\n", findmax_timer);
  /*================================================================================*/
  tstart= start(tstart);
  printf("Parallel max value is:\t%10ld\n", result);
  double print_timer = stop(tstart, tend);
  //printf("Time for printing output %g\n", print_timer);
  printf("Total time for multi-thread %g\n", (alloc_timer + create_timer + join_timer + findmax_timer + print_timer));
  
  // Cleanup
  free(thread_info);
  thread_info = NULL;
  free(max_values);
  max_values = NULL;
  pthread_mutex_destroy(&result_mutex);
  free(threads);
  threads = NULL;
  free(thread_ids);
  thread_ids = NULL;
  free(values);
  values = NULL;
  exit(0);
}
