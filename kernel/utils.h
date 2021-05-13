#include <hls_stream.h>

#ifndef UTILS_HEADER
#define UTILS_HEADER

#define PRAGMA_SUB(x) _Pragma (#x)
#define PRAGMA(x) PRAGMA_SUB(x)

#define STREAM(a, b) PRAGMA_SUB(HLS STREAM variable=a depth=b) \
    PRAGMA_SUB(HLS BIND_STORAGE variable=a type=FIFO impl=LUTRAM)

template <class T, int N>
void duplicateStream(hls::stream<T> & source, hls::stream<T> target[N], unsigned int number) {
  number_loop:
  for (unsigned int i=0;i<number;i++) {
#pragma HLS PIPELINE II=1
    T data=source.read();
    duplicates_loop:
    for (unsigned int j=0;j<N;j++) {
#pragma HLS UNROLL
      target[j].write(data);
    }
  }
}

static unsigned int get_chunk_size(unsigned int chunk_num, unsigned int num_chunks, unsigned int max_chunk_length, unsigned int remainder) {
  return chunk_num < num_chunks - 1 ? max_chunk_length : (remainder > 0 ? remainder : max_chunk_length);
}

/*
 * This is a little complex, as we need to grab the extra size in Y based on the additional halos (two per chunk), so the total data size depends on the number
 * of chunks. However, if in doing this we cross into a new chunk boundary then we also need to take that into account, and likewise in doing that if we cross
 * over into another need to take that into account etc... So it is iteratively finding the total number until the number of chunks is stable between two
 * iterations and we have found the total number.
 */
static unsigned int get_number_y_access_with_overlap(unsigned int size_y) {
  unsigned int latest_size_in_y=size_y - 2;
  unsigned int processed_chunks=1;
  y_total_size_search_loop:
  while (true) {
#pragma HLS PIPELINE II=5
    // Not ideal to pipeline with II=5, but only called once so won't be massive performance overhead
    unsigned int number_chunks=latest_size_in_y / MAX_Y_SIZE;
    unsigned int remainder=latest_size_in_y - (number_chunks * MAX_Y_SIZE);
    if (remainder > 0) number_chunks++;
    if (number_chunks == processed_chunks) break;

    unsigned int extra_overlap=2 * (number_chunks-processed_chunks);
    latest_size_in_y=latest_size_in_y + extra_overlap;
    processed_chunks=number_chunks;
  }
  return latest_size_in_y;
}

static unsigned int get_number_chunks(unsigned int size_y) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;
  return number_chunks;
}

#endif
