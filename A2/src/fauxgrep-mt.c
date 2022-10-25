// Setting _DEFAULT_SOURCE is necessary to activate visibility of
// certain header file contents on GNU/Linux systems.
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fts.h>

// err.h contains various nonstandard BSD extensions, but they are
// very handy.
#include <err.h>

#include <pthread.h>

#include "job_queue.h"

pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
const char *common_needle;

int fauxgrep_mt_file(char const *needle, char const *path) {
  FILE *f = fopen(path, "r");

  if (f == NULL) {
    warn("failed to open %s", path);
    return -1;
  }

  char *line = NULL;
  size_t linelen = 0;
  int lineno = 1;

  while (getline(&line, &linelen, f) != -1) {
    if (strstr(line, needle) != NULL) {
      pthread_mutex_lock(&stdout_mutex);
      printf("%s:%d: %s", path, lineno, line);
      pthread_mutex_unlock(&stdout_mutex);

    }

    lineno++;
  }

  free(line);
  fclose(f);

  return 0;
}

void* worker (void* arg) {
  struct job_queue *jq = arg;

  while (1) {
    const char *nextpath;
    if (job_queue_pop(jq, (void**)&nextpath) == 0) {
      fauxgrep_mt_file(common_needle, nextpath);
      free((void*)nextpath);
    } else {
      break; // Shuts down the workers.
    }
  }
  return NULL;
}
 
int main(int argc, char * const *argv) {
  printf("test\n");
  if (argc < 2) {
    printf("test0\n");
    err(1, "usage: [-n INT] STRING paths...");
    exit(1);
  }

  int num_threads = 1;
  char const *needle = argv[1];
  common_needle = needle;
  char * const *paths = &argv[2];


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

    needle = argv[3];
    paths = &argv[4];

  } else {
    needle = argv[1];
    paths = &argv[2];
  }

  // Initialises a job_queue.
  struct job_queue jq;
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
      //fauxgrep_mt_file(needle, p->fts_path);
      break;
    default:
      break;
    }
  }

  fts_close(ftsp);
  job_queue_destroy(&jq); // Shuts down the job queue.

  return 0;
}
