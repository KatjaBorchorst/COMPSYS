#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

/*
DESCRIPTION
----------------------------------------------
SHOULD OPERATE BY PERFORMING A LINEAR SEARCH
THROUGH ALL RECORDS FOR THE DESCIRED ID.
*/


struct naive_data {
  struct record *rs;
  int n;
};

struct naive_data* mk_naive(struct record* rs, int n) {
  /*int i;
  n = 19999;
  for (i = 1; i < n; ++i) {
    printf("housenumber: %lld \n", rs[i].osm_id); 
    printf("lon: %f \n", rs[i].lon);
    printf("lat: %f \n", rs[i].lat);
    printf("name: %s \n", rs[i].name);
    printf("data: %s %d %d \n", rs[i].name, rs[i].lat, rs[i].lon);
    printf("\n");
  }*/
  //return 0;
}

void free_naive(struct naive_data* data) {
  free(data->rs);
	free(data);
}

const struct record* lookup_naive(struct naive_data *data, int64_t needle) {
  int c = 20000;
  int i;
  struct record* rs;
  for (i = 1; i < c; ++i) {
    if (data == needle) {
      printf("%i: %lld %d %d \n", data, rs[i].name, rs[i].lat, rs[i].lon); //NUMBER: name, osm_id, coordinates
    }
  }
  //assert(0);

	return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_naive,
                    (free_index_fn)free_naive,
                    (lookup_fn)lookup_naive);
}
