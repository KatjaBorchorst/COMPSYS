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
  struct kd_tree_node *node;
  int depth;
  
};


struct kd_tree_node {
  int axis;
  double point[2];
  struct record *left;
  struct record *right;
};
 
//Comparer functions
double compareLon (const void * a, const void * b) {
   const struct record *first = a;
   const struct record *second = b;
   return (second->lon < first->lon ) - ( first->lon < second->lon );
}

double compareLat (const void * a, const void * b) {
   const struct record *first = a;
   const struct record *second = b;
   return (second->lat < first->lat ) - ( first->lat < second->lat );
}

struct kdtree_data* mk_kdtree(struct record* rs, int n) {
  int depth = floor(log2(n))+1; //if not 0-indexed depth
  int axis = depth % 2;
  struct record median;

  if (!axis) {
    qsort(rs, n, sizeof(struct record), compareLon); //mutable?
    median = rs[n/2];
  } else if (axis) {
    qsort(rs, n, sizeof(struct record), compareLat);
    median = rs[n/2];
  }

  struct kd_tree_node *node = malloc(sizeof(struct kd_tree_node));
  assert(node != NULL); //check malloc return value

  node->axis = axis;
  node->point[0] = median.lon;
  node->point[1] = median.lat;  
  node->left = mk_kdtree()
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
