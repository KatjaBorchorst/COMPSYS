#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "coord_query.h"

struct kdtree_data {
  struct record *rs;
  struct record *left;
  struct record *right;
  
};

struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  assert(0);
  // TODO
}

void free_kdtree(struct kdtree_data* data) {
  assert(0);
  // TODO
}

const struct record* lookup_kdtree(struct kdtree_data *data, double lon, double lat) {
  assert(0);
  // TODO
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
