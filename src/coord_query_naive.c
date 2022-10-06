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

struct naive_data {
    struct record *rs;
    int n;
};

struct naive_data* mk_naive(struct record* rs, int n) {
    struct naive_data* arr = malloc(sizeof(struct naive_data));
    arr->n = n;
    arr->rs = rs;
    return arr;
}

void free_naive(struct naive_data* data) {
  if (data == NULL) {
    free(data);
  }
  if (data->rs != NULL) {
    free(data->rs);
  }
  free(data);
}

double distance(double lon1, double lat1, double lon2, double lat2) {
  return sqrt(pow((lon1 - lon2), 2.0) + pow((lat1 - lat2), 2.0));
}

const struct record* lookup_naive(struct naive_data *data, double lon, double lat) {
  int closest = 0;
  for (int i = 0; i < data->n; ++i) {
      double dist = distance(lon, lat, data->rs[i].lon, data->rs[i].lat);
      if (dist < (distance(lon, lat, data->rs[closest].lon, data->rs[closest].lat))) {
        closest = i;
      }
  }
  return &data->rs[closest];
}

int main(int argc, char** argv) {
  return coord_query_loop(argc, argv,
                          (mk_index_fn)mk_naive,
                          (free_index_fn)free_naive,
                          (lookup_fn)lookup_naive);
}
