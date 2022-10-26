#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <err.h>

#include "job_queue.h"


int job_queue_init(struct job_queue *job_queue, int capacity) {
  if (job_queue == NULL){
    return 1;
  }
  job_queue->capacity = capacity;
  job_queue->size = 0;
  job_queue->top = 0;
  job_queue->bottom = 0;
  job_queue->die = 0;
  pthread_mutex_init(&job_mutex, NULL);
  pthread_cond_init(&empty_cond, NULL);
  pthread_cond_init(&filled_cond, NULL);
  pthread_cond_init(&die_cond, NULL);
  return 0;
}

int job_queue_destroy(struct job_queue *job_queue) {
  printf("destroyed\n");
  assert(pthread_mutex_lock(&job_mutex) == 0);
  job_queue->die = 1;
  while (job_queue->size != 0) {
    assert(pthread_cond_wait(&empty_cond, &job_mutex) == 0);
  }
  assert(pthread_cond_broadcast (&filled_cond) == 0); // Wake pop if it is awaiting input. 
                                                      // (destroy called after last pop)
  assert(pthread_cond_wait(&die_cond, &job_mutex) == 0);
  //wait for 
  assert(pthread_cond_destroy(&filled_cond) == 0);
  assert (pthread_cond_destroy(&empty_cond) == 0);
  assert(pthread_cond_destroy(&die_cond) == 0);
  assert(pthread_mutex_unlock(&job_mutex) == 0); // Unlock before destroying.
  assert(pthread_mutex_destroy(&job_mutex) == 0);
  return 0;
}

int job_queue_push(struct job_queue *job_queue, void *data) {
  printf("push\n"); // HUSK AT FJERNE.
  assert (pthread_mutex_lock(&job_mutex) == 0);
  if (job_queue->die == 1) {
    err(1, "pushing to destroyed queue");
    return 1;
  }
  while (job_queue->capacity <= job_queue->size) {
    printf("waiting for less full queue");
    assert(pthread_cond_wait(&empty_cond, &job_mutex) == 0);
  }
  if (job_queue->size != 0) {
    job_queue->bottom++;
  }
  job_queue->data[job_queue->bottom] = data;
  job_queue->size ++;
  assert(pthread_mutex_unlock(&job_mutex) == 0);
  assert(pthread_cond_broadcast(&filled_cond) == 0);
  return 0;
}

int job_queue_pop(struct job_queue *job_queue, void **data) {
  printf("pop\n");
  assert(pthread_mutex_lock(&job_mutex) == 0);
  while (job_queue->size == 0 && job_queue->die == 0) {
      printf("waiting for queue to be filled\n");
      assert(pthread_cond_wait(&filled_cond, &job_mutex) == 0);
  } 
  if (job_queue->size == 0 && job_queue->die == 1) {
    pthread_mutex_unlock(&job_mutex);
    pthread_cond_broadcast(&die_cond);
    return -1;
  }
  *data = job_queue->data[job_queue->top];
  if (job_queue->top != job_queue->bottom) {
    job_queue->top ++;
  }
  job_queue->size --;
  assert(pthread_mutex_unlock(&job_mutex) == 0);
  assert(pthread_cond_broadcast(&empty_cond) == 0);
  return 0;
}
