#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>
#include <math.h>

#include "record.h"
#include "coord_query.h"

struct kdtree_data{
  struct record *rs;
  struct kd_tree_node *rootnode;
};

struct kd_tree_node{
  int axis;          
  struct kd_tree_node *left;  // child
  struct kd_tree_node *right; // child
  struct record *record;      // record
};


int compareLon(const void *a, const void *b) {
  double first = ((struct record *)a)->lon;
  double second = ((struct record *)b)->lon;
  if (first < second){
    return -1;
  } else if (first > second){
    return 1;
  } else {
    return 0;
  }
}

int compareLat(const void *a, const void *b) {
  double first = ((struct record *)a)->lat;
  double second = ((struct record *)b)->lat;
  if (first < second){
    return -1;
  } else if (first > second){
    return 1;
  } else {
    return 0;
  }
}

struct kd_tree_node *kdtree(struct record *rs, int depth, int n){
  if (n == 0){
    return NULL;
  }
  int axis = depth % 2; 
  // select median by axis from records
  struct record median;
  if (axis == 0){
    qsort(rs, n, sizeof(struct record), (int (*)(const void *, const void *))compareLon); // rs is sorted
    median = rs[n/2];
  }
  else if (axis == 1){
    qsort(rs, n, sizeof(struct record), (int (*)(const void *, const void *))compareLat);
    median = rs[n/2];
  }
  struct kd_tree_node *node = malloc(sizeof(struct kd_tree_node)); // node = new node
  assert(node != NULL);                                            // check malloc return value

  node->axis = axis;        
  node->record = &median;
  node->left = kdtree(rs, depth + 1, (n / 2.0));
  node->right = kdtree((rs + (n / 2) + 1), depth + 1, n - (n / 2) - 1);
  return node; 
}

struct kdtree_data *mk_kdtree(struct record *rs, int n){
  struct kdtree_data *data = malloc(sizeof(struct kdtree_data));
  struct record *recs = calloc(n ,sizeof(struct record));
  memcpy(recs, rs, n * (sizeof(struct record))); //copy rs into recs
  data->rs = recs;
  data->rootnode = kdtree(recs, 0, n);
  return data;
}

void deleteNodes(struct kd_tree_node *node){
  if (node == NULL) {
    return;
  }
  deleteNodes(node->left);
  deleteNodes(node->right);
  free(node);
}

void free_kdtree(struct kdtree_data *data){
  deleteNodes(data->rootnode);
  free(data);
  free(data->rs);
}

double distance(double lon1, double lat1, double lon2, double lat2){
  double result = sqrt((lon1 - lon2) * (lon1 - lon2) + (lat1 - lat2) * (lat1 - lat2));
  return result;
}

struct record* lookup (struct record *closest, double qlon, double qlat, struct kd_tree_node *node, double radius){
  struct record *closest_record = closest;
  double distance_node_query = distance(qlon, qlat, node->record->lon, node->record->lat);
  if (node == NULL) {
    return closest_record;
  } else if (radius > distance_node_query){
    closest_record = node->record;
  }

  double diff;
  if (node->axis == 0) {
    diff = (node->record->lon) - qlon;
  } else if (node->axis == 1) {
    diff = (node->record->lat) - qlat;
  }

  double updated_radius = distance(qlon, qlat, closest_record->lon, closest_record->lat);

  if ((node->left != NULL) && (diff >= 0 || updated_radius > fabs(diff))) {
    lookup(closest_record, qlon, qlat, node->left, updated_radius);
  }
  if ((node->right != NULL) && (diff <= 0 || updated_radius > fabs(diff))) {
    lookup(closest_record, qlon, qlat, node->right, updated_radius);
  }
  return closest_record;
}

const struct record *lookup_kdtree(struct kdtree_data *data, double lon, double lat){
  double radius = INFINITY;
  struct record *closest = NULL;
  return lookup(closest, lon, lat, data->rootnode, radius);
}

int main(int argc, char **argv){
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_kdtree,
                          (free_index_fn)free_kdtree,
                          (lookup_fn)lookup_kdtree);
}
