#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

#include <pthread.h>

pthread_mutex_t job_mutex;
pthread_cond_t empty_cond;
pthread_cond_t filled_cond;
pthread_cond_t die_cond;

struct job_queue {
  int size;
  int capacity;
  int top;
  int bottom;
  int die;
  void* data[40000]; //size of array has to be able to contain all data, since the top is not moved
                    //back to 0, when elements popped. 40000 is chosen because of of fibs testinputs
};

// Initialise a job queue with the given capacity.  The queue starts out
// empty.  Returns non-zero on error.
int job_queue_init(struct job_queue *job_queue, int capacity);

// Destroy the job queue.  Blocks until the queue is empty before it
// is destroyed.
int job_queue_destroy(struct job_queue *job_queue);

// Push an element onto the end of the job queue.  Blocks if the
// job_queue is full (its size is equal to its capacity).  Returns
// non-zero on error.  It is an error to push a job onto a queue that
// has been destroyed.
int job_queue_push(struct job_queue *job_queue, void *data);

// Pop an element from the front of the job queue.  Blocks if the
// job_queue contains zero elements.  Returns non-zero on error.  If
// job_queue_destroy() has been called (possibly after the call to
// job_queue_pop() blocked), this function will return -1.
int job_queue_pop(struct job_queue *job_queue, void **data);

#endif
