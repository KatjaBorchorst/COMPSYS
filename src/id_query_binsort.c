#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include "record.h"
#include "id_query.h"

struct index_record {
  int64_t osm_id;
  const struct record *record; //record
};

struct indexed_data{
  struct index_record *irs; //index_record array
  int n;
};

int compareID (const void * a, const void * b) {
   const struct index_record *first = a;
   const struct index_record *second = b;
   return (second->osm_id < first->osm_id ) - ( first->osm_id < second->osm_id );
}

struct indexed_data* mk_indexed(struct record* rs, int n){
  struct indexed_data* data = malloc(sizeof(struct indexed_data));
  data->n = n;
  struct index_record *irs = malloc(n*sizeof(struct index_record));
  for (int i = 0; i <= n; i++){
    irs[i].osm_id = rs[i].osm_id;
    irs[i].record = &rs[i];
  }
  data->irs = irs;
  qsort(data->irs, data->n, sizeof(struct index_record), compareID);
  return data;
}

void free_indexed(struct indexed_data* data) {
  free(data->irs);
  free(data);
}

const struct record* lookup_indexed(struct indexed_data *data, int64_t needle) {
  int low = 0;
  int high = (data->n)-1;
  int mid = 0;
  while(low <= high) {
    mid = (high+low)/2;
    if (data->irs[mid].osm_id < needle) {
      low = mid+1;
    } else if (data->irs[mid].osm_id > needle) {
      high = mid-1;
    } else {
      return data->irs[mid].record;
    }
  }
  return NULL;
}

int main(int argc, char** argv) {
  return id_query_loop(argc, argv,
                    (mk_index_fn)mk_indexed,
                    (free_index_fn)free_indexed,
                    (lookup_fn)lookup_indexed);
}
