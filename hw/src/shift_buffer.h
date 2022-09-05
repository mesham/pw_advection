#include "definitions.h"
#include <hls_stream.h>
#include <stdio.h>
#include "utils.h"

void shift_buffer_for_chunk(hls::stream<REAL_TYPE>&, hls::stream<struct stencil_data>&, unsigned int, unsigned int, unsigned int);

void shift_buffer(hls::stream<REAL_TYPE> & data_stream, hls::stream<struct stencil_data> & stencil_stream,
    unsigned int x_size, unsigned int y_size, unsigned int z_size) {

  unsigned int number_access_y=get_number_y_access_with_overlap(y_size);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size=get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    shift_buffer_for_chunk(data_stream, stencil_stream, x_size, chunk_size, z_size);
  }
}

void shift_buffer_for_chunk(hls::stream<REAL_TYPE> & data_stream, hls::stream<struct stencil_data> & stencil_stream,
    unsigned int x_size, unsigned int y_size, unsigned int z_size) {

  REAL_TYPE level3_cache[3][MAX_Y_SIZE][MAX_Z_SIZE];
  REAL_TYPE level2_cache[3][3][MAX_Z_SIZE];

#pragma HLS array_partition variable=level3_cache complete dim=1
#pragma HLS array_partition variable=level2_cache complete dim=1
#pragma HLS array_partition variable=level2_cache complete dim=2

  int num_writes=0;

  unsigned int i,j,k,r;
  struct stencil_data stencil;
  REAL_TYPE current_vals[3][3][3];
#pragma HLS array_partition variable=current_vals complete	

  x_loop:
  for(i=0 ; i<x_size+1; i++) {
    y_loop:
    for(j=0 ; j<y_size+3; j++) {
      z_loop:
      for(k=0 ; k<z_size+3; k++) {
#pragma HLS PIPELINE II=1

        if (i < x_size) {
          if (i < 3 && j < y_size && k < z_size) {
            REAL_TYPE element=data_stream.read();
            level3_cache[i][j][k]=element;
          } else if (i > 2 && j > 2 && k > 2) {
            level3_cache[0][j-3][k-3]=level3_cache[1][j-3][k-3];
            level3_cache[1][j-3][k-3]=level3_cache[2][j-3][k-3];
            level3_cache[2][j-3][k-3]=data_stream.read();
          }
        }

        if (i > 2 && j < y_size && k > 2) {
          for (r=0;r<3;r++) {
            level2_cache[r][0][k-3]=level2_cache[r][1][k-3];
            level2_cache[r][1][k-3]=level2_cache[r][2][k-3];
            level2_cache[r][2][k-3]=level3_cache[r][j][k-3];
          }
        }

        if (i > 2 && j > 2 && j < y_size + 1) {
          if (k < z_size+1) {
            ni_loop:
            for (unsigned int ni=0;ni<3;ni++) {
              nj_loop:
              for (unsigned int nj=0;nj<3;nj++) {
                nk_loop:
                for (unsigned int nk=0;nk<3;nk++) {
                  REAL_TYPE d;
                  if (nk == 2) {
                    if (k < z_size) d=level2_cache[ni][nj][k];
                  } else {
                    d=current_vals[ni][nj][nk+1];
                  }
                  current_vals[ni][nj][nk]=d;
                  stencil.values[ni][nj][nk]=d;
                }
              }
            }
            if (k > 1) {
              num_writes++;
              stencil_stream.write(stencil);
            }
          }
        }
      }
    }
  }
}
