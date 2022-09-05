#include "definitions.h"
#include <hls_stream.h>
#include <stdio.h>
#include "utils.h"

static void advect_u_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    hls::stream<REAL_TYPE>&, REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int);
static void advect_v_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    hls::stream<REAL_TYPE>&, REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int);
static void advect_w_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    hls::stream<REAL_TYPE>&, REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int);

void advect_u(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream, 
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y= get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    advect_u_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, result_stream, tzc1, tzc2, tcx, tcy, size_x, chunk_size_y, size_z);
  }
}

static void advect_u_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  x_loop:
  for (unsigned int i=2;i<size_x-2;i++) {
    y_loop:
    for (unsigned int j=1;j<size_y-1;j++) {
      z_loop:
      for (unsigned int k=1;k<size_z;k++) {
#pragma HLS PIPELINE II=1

        struct stencil_data u_stencil = u_stencil_stream.read();
        struct stencil_data v_stencil = v_stencil_stream.read();
        struct stencil_data w_stencil = w_stencil_stream.read();

        double su_x=tcx*(u_stencil.values[0][1][1] * (u_stencil.values[1][1][1] + u_stencil.values[0][1][1]) - u_stencil.values[2][1][1] * (u_stencil.values[1][1][1] + u_stencil.values[2][1][1]));
        double su_y=tcy*(u_stencil.values[1][0][1] * (v_stencil.values[1][0][1] + v_stencil.values[1][0][1]) - u_stencil.values[1][2][1] * (v_stencil.values[1][1][1] + v_stencil.values[2][1][1]));

        double su_z;
        if (k < size_z - 1) {
          su_z=tzc1[k] * u_stencil.values[1][1][0] * (w_stencil.values[1][1][0] + w_stencil.values[2][1][0]) - tzc2[k] * u_stencil.values[1][1][2] * (w_stencil.values[1][1][1] + w_stencil.values[2][1][1]);
        } else {
          // Lid
          su_z=tzc1[k] * u_stencil.values[1][1][0] * (w_stencil.values[1][1][0] + w_stencil.values[2][1][0]);
        }
        result_stream.write(su_x+su_y+su_z);
      }
    }
  }
}

void advect_v(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y= get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    advect_v_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, result_stream, tzc1, tzc2, tcx, tcy, size_x, chunk_size_y, size_z);
  }
}

static void advect_v_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  x_loop:
  for (unsigned int i=2;i<size_x-2;i++) {
    y_loop:
    for (unsigned int j=1;j<size_y-1;j++) {
      z_loop:
      for (unsigned int k=1;k<size_z;k++) {
#pragma HLS PIPELINE II=1

        struct stencil_data u_stencil = u_stencil_stream.read();
        struct stencil_data v_stencil = v_stencil_stream.read();
        struct stencil_data w_stencil = w_stencil_stream.read();

        double sv_y=tcy*(v_stencil.values[1][0][1] * (v_stencil.values[1][1][1] + v_stencil.values[1][0][1]) - v_stencil.values[1][2][1] * (v_stencil.values[1][1][1] + v_stencil.values[1][2][1]));
        double sv_x=tcx*(v_stencil.values[0][1][1] * (u_stencil.values[0][1][1] + u_stencil.values[1][2][1]) - v_stencil.values[2][1][1] * (u_stencil.values[1][1][1] + u_stencil.values[1][2][1]));

        double sv_z;
        if (k < size_z - 1) {
          sv_z=tzc1[k] * v_stencil.values[1][1][0] * (w_stencil.values[1][1][0] + w_stencil.values[1][2][0]) - tzc2[k] * v_stencil.values[1][1][2] * (w_stencil.values[1][1][1] + w_stencil.values[1][2][1]);
        } else {
          // Lid
          sv_z=tzc1[k] * u_stencil.values[1][1][0] * (w_stencil.values[1][1][0] + w_stencil.values[1][2][0]);
        }
        result_stream.write(sv_y+sv_x+sv_z);
      }
    }
  }
}

void advect_w(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzd1[MAX_Z_SIZE], REAL_TYPE tzd2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y=get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    advect_w_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, result_stream, tzd1, tzd2, tcx, tcy, size_x, chunk_size_y, size_z);
  }
}

static void advect_w_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    hls::stream<REAL_TYPE> & result_stream, REAL_TYPE tzd1[MAX_Z_SIZE], REAL_TYPE tzd2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  x_loop:
  for (unsigned int i=2;i<size_x-2;i++) {
    y_loop:
    for (unsigned int j=1;j<size_y-1;j++) {
      z_loop:
      for (unsigned int k=1;k<size_z;k++) {
#pragma HLS PIPELINE II=1

        struct stencil_data u_stencil = u_stencil_stream.read();
        struct stencil_data v_stencil = v_stencil_stream.read();
        struct stencil_data w_stencil = w_stencil_stream.read();

        if (k < size_z - 1) {
          // This makes it easier for the shift buffer, as one implementation for all three
          double sw_z=tzd1[k] * w_stencil.values[1][1][0] * (w_stencil.values[1][1][1] + w_stencil.values[1][1][0]) - tzd2[k] * w_stencil.values[1][1][2] * (w_stencil.values[1][1][1] + w_stencil.values[1][1][2]);
          double sw_x=tcx*(w_stencil.values[0][1][1]*(u_stencil.values[1][1][1] + u_stencil.values[0][1][2]) - w_stencil.values[2][1][1] * (u_stencil.values[1][1][1] + u_stencil.values[1][1][2]));
          double sw_v=tcy*(w_stencil.values[1][0][1] * (v_stencil.values[1][0][1] + v_stencil.values[1][0][2]) - w_stencil.values[1][2][1] * (v_stencil.values[1][1][1] + v_stencil.values[1][1][2]));

          result_stream.write(sw_z+sw_x+sw_v);
        } else {
          result_stream.write(0); // Ignore top
        }
      }
    }
  }
}
