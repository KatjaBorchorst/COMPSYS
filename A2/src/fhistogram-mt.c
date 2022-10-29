// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

#include "job_queue.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
const char *common_needle;

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include "histogram.h"

int fhistogram_mt_file(char const *path) {
  FILE *f = fopen(path, "r");

  int local_histogram[8] = {0}; 

  if (f == NULL) {
    fflush(stdout);
    warn("failed to open %s", path);
    return -1;
  }

  int i = 0

  char c;
  while(fread(&c, sizeof(c), 1, f) == 1) 
  {
    pthread_mutex_lock(&stdout_mutex);
    i++;
    update_histogram(local_hiatogram, c);
    pthread_mutex_unlock(&stdout_mutex);

    if ((i % 100000) == 0) {
      pthread_mutex_lock(&stdout_mutex);
      merge_histogram(local_histogram, c);
      print_histogram(global_histogram);
      pthread_mutex_unlock(&stdout_mutex);
    }
  }

  fclose(f);
  pthread_mutex_lock(&stdout_mutex);
  merge_histogram(local_histogram, global_histogram);
  print_histogram(global_histogram);
  pthread_mutex_unlock(&stdout_mutex);

  return 0;
}


void* worker (void* arg) {
  job_queue *jq = arg;

  while (1) {
    char *line;
    if (job_queue_pop(jq, (void**)&line) == 0) {
      fhistogram(line);
      free(line);
    } else {
      break;
    }
  }
  return NULL;
}

int main(int argc, char * const *argv) {
  if (argc < 2) {
    err(1, "usage: paths...");
    exit(1);
  }

  int num_threads = 1;
  char * const *paths = &argv[1];

  if (argc > 3 && strcmp(argv[1], "-n") == 0) {
    // Since atoi() simply returns zero on syntax errors, we cannot
    // distinguish between the user entering a zero, or some
    // non-numeric garbage.  In fact, we cannot even tell whether the
    // given option is suffixed by garbage, i.e. '123foo' returns
    // '123'.  A more robust solution would use strtol(), but its
    // interface is more complicated, so here we are.
    num_threads = atoi(argv[2]);

    if (num_threads < 1) {
      err(1, "invalid thread count: %s", argv[2]);
    }

    paths = &argv[3];
  } else {
    paths = &argv[1];
  }

  // Initialise the job queue and some worker threads here.
  job_queue jq;
  job_queue_init(&jq, 64);


  // Launching n worker threads.
  int i;
  int n = num_threads;
  pthread_t threads[n];
  for (i = 0; i < n; i++) {
    if (pthread_create(&threads[i], NULL, worker, &jq) != 0) {
      err(1, "pthread_create() failed");
    }
  }


  // FTS_LOGICAL = follow symbolic links
  // FTS_NOCHDIR = do not change the working directory of the process
  //
  // (These are not particularly important distinctions for our simple
  // uses.)
  int fts_options = FTS_LOGICAL | FTS_NOCHDIR;

  FTS *ftsp;
  if ((ftsp = fts_open(paths, fts_options, NULL)) == NULL) {
    err(1, "fts_open() failed");
    return -1;
  }

  FTSENT *p;
  while ((p = fts_read(ftsp)) != NULL) {
    switch (p->fts_info) {
    case FTS_D:
      break;
    case FTS_F:
      job_queue_push(&jq, strdup(p->fts_path)); // Process the file p->fts_path, somehow.
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);

  job_queue_destroy(&jq); // Shut down the job queue and the worker threads here.
  for (int i = 0; i < num_threads; i++) {
    if (pthread_join(threads[i], NULL) ! = 0) {
      err(1, "pthread_join() faild");
    }
  }


  move_lines(9);

  return 0;
}
