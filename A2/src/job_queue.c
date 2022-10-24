#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "job_queue.h"

pthread_mutex_t job_mutex;
pthread_cond_t empty_cond;
pthread_cond_t filled_cond;

int job_queue_init(struct job_queue *job_queue, int capacity) {
  job_queue->capacity = capacity;
  job_queue->size = 0;
  job_queue->top = -1;
  job_queue->bottom = -1;
  job_queue->die = 0;
  assert(pthread_mutex_init (&job_mutex, NULL) == 0);
  assert(pthread_cond_init (&filled_cond, NULL) == 0);
  assert(pthread_cond_init (&empty_cond, NULL) == 0);
  return 0;
}

int isEmpty (struct job_queue *job_queue) {
  assert(pthread_mutex_lock(&job_mutex) == 0);
  if (job_queue->size == 0) {
    return 1;
  } else {
    return 0;
  }
  assert(pthread_mutex_unlock(&job_mutex) == 0);
}

int isFull (struct job_queue *job_queue) {
  assert (pthread_mutex_lock(&job_mutex) == 0);
  if (job_queue->capacity > job_queue->size) {
    printf("isnotfull");
    return 0;
  } else {
      printf("isfull");
    return 1;
  }
  assert(pthread_mutex_unlock(&job_mutex) == 0);
}

int job_queue_destroy(struct job_queue *job_queue) {
    printf("destroyed\n");
  assert(pthread_mutex_lock(&job_mutex) == 0);
  job_queue->die = 1;
  while (!isEmpty(job_queue)) {
    pthread_cond_wait(&empty_cond, &job_mutex);
  }
  assert(pthread_cond_broadcast (&filled_cond) == 0);
  assert(pthread_cond_destroy(&filled_cond) == 0);
  assert (pthread_cond_destroy(&empty_cond) == 0);
  assert(pthread_mutex_destroy(&job_mutex) == 0);
  assert(pthread_mutex_unlock(&job_mutex) == 0);
  return 0;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  printf("push\n");
  assert (pthread_mutex_lock(&job_mutex) == 0);
  if (job_queue->die == 1) {
    return 1;
  }
  printf("isfull retval: %d", isFull(job_queue));
  while (isFull(job_queue)) {
    assert(pthread_cond_wait(&empty_cond, &job_mutex) == 0);
  }
  printf("got past cond\n");
  job_queue->bottom ++;
  job_queue->data[job_queue->bottom] = data;
  if (isEmpty(job_queue)) {
    job_queue->top ++;
  }
  job_queue->size ++;
  printf("size: %d", job_queue->size);

  assert(pthread_mutex_unlock(&job_mutex) == 0);
  assert(pthread_cond_broadcast(&filled_cond) == 0);
  return 0;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  printf("pop\n");
  assert(pthread_mutex_lock(&job_mutex) == 0);
  while (isEmpty(job_queue) && !job_queue->die) {
      assert(pthread_cond_wait(&filled_cond, &job_mutex) == 0);
  } 
  if (isEmpty(job_queue) && job_queue->die) {
    return -1;
  }
  *data = job_queue->data[job_queue->top]; //pointer??
  job_queue->top --;
  job_queue->size --;
  printf("top: %d", job_queue->top);
  assert(pthread_mutex_unlock(&job_mutex) == 0);
  assert(pthread_cond_signal(&empty_cond) == 0);
  return 0;
}
