#include "definitions.h"
#include <hls_stream.h>
#include <stdio.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include "utils.h"

static void stream_to_aie_for_advect_u_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    REAL_TYPE [MAX_Z_SIZE], REAL_TYPE [MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int,
    hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&,
    hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&);
static void stream_to_aie_for_advect_v_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    REAL_TYPE [MAX_Z_SIZE], REAL_TYPE [MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int,
    hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&,
        hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&);
static void stream_to_aie_for_advect_w_chunk(hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&, hls::stream<struct stencil_data>&,
    REAL_TYPE [MAX_Z_SIZE], REAL_TYPE [MAX_Z_SIZE], double, double, unsigned int, unsigned int, unsigned int,
    hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&,
        hls::stream<qdma_axis<128,0,0,0> >&, hls::stream<qdma_axis<128,0,0,0> >&);

ap_uint<32> pack_val(REAL_TYPE value) {
  float v=(float) value;
  return *((ap_uint<32>*) &(v));
}

void init_aie(unsigned int size_x, unsigned int size_y, unsigned int size_z, hls::stream<qdma_axis<128,0,0,0> >& aie_stream) {
  qdma_axis<128,0,0,0> num_els_container;
  ap_uint<128> num_els;
  float num_els_f=(size_x-4) * (size_y-4) * (size_z-1);
  num_els.range(31,0)=*((ap_uint<32>*) &(num_els_f));
  num_els_container.data=num_els;
  aie_stream.write(num_els_container);
}

void stream_to_aie_for_advect_u(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream, 
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& u_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& u_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& u_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_mul_1_stream) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  //init_aie(size_x, size_y, size_z, u_lhs_0_stream);
  //init_aie(size_x, size_y, size_z, u_lhs_1_stream);
  //init_aie(size_x, size_y, size_z, u_mul_0_stream);
  //init_aie(size_x, size_y, size_z, u_mul_1_stream);

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y= get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    stream_to_aie_for_advect_u_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, tzc1, tzc2, tcx, tcy, size_x, chunk_size_y, size_z, u_lhs_0_stream, u_rhs_0_stream,
        u_lhs_1_stream, u_rhs_1_stream, u_mul_0_stream, u_mul_1_stream);
  }
}

static void stream_to_aie_for_advect_u_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& u_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& u_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& u_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& u_mul_1_stream) {
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

        qdma_axis<128,0,0,0> a1, a2, b1, b2;
        ap_uint<128> a1_d, a2_d, b1_d, b2_d;
        float empty=0.0;

        a1.keep_all();
        a2.keep_all();
        b1.keep_all();
        b2.keep_all();

        a1_d.range(31,0)=pack_val(u_stencil.values[1][1][1]);
        b1_d.range(31,0)=pack_val(u_stencil.values[0][1][1]);

        a1_d.range(63,32)=pack_val(u_stencil.values[1][1][1]);
        b1_d.range(63,32)=pack_val(u_stencil.values[2][1][1]);

        a1_d.range(95,64)=pack_val(v_stencil.values[1][0][1]);
        b1_d.range(95,64)=pack_val(v_stencil.values[1][0][1]);

        a1_d.range(127,96)=pack_val(v_stencil.values[1][1][1]);
        b1_d.range(127,96)=pack_val(v_stencil.values[2][1][1]);

        a2_d.range(31,0)=pack_val(w_stencil.values[1][1][0]);
        b2_d.range(31,0)=pack_val(w_stencil.values[2][1][0]);

        a2_d.range(63,32)=pack_val(w_stencil.values[1][1][1]);
        b2_d.range(63,32)=pack_val(w_stencil.values[2][1][1]);

        a2_d.range(95,64)=pack_val(empty);
        b2_d.range(95,64)=pack_val(empty);

        a2_d.range(127,96)=pack_val(empty);
        b2_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        a2.data=a2_d;
        b1.data=b1_d;
        b2.data=b2_d;

        u_lhs_0_stream.write(a1);
        u_lhs_1_stream.write(a2);
        u_rhs_0_stream.write(b1);
        u_rhs_1_stream.write(b2);

        a1_d.range(31,0)=pack_val(u_stencil.values[0][1][1]);
        b1_d.range(31,0)=pack_val(u_stencil.values[1][1][0]);

        a1_d.range(63,32)=pack_val(u_stencil.values[2][1][1]);
        b1_d.range(63,32)=pack_val(u_stencil.values[1][1][2]);

        a1_d.range(95,64)=pack_val(u_stencil.values[1][0][1]);
        b1_d.range(95,64)=pack_val(empty);

        a1_d.range(127,96)=pack_val(u_stencil.values[1][2][1]);
        b1_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        b1.data=b1_d;

        u_mul_0_stream.write(a1);
        u_mul_1_stream.write(b1);
      }
    }
  }
}

void stream_to_aie_for_advect_v(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& v_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& v_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_1_stream) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  //init_aie(size_x, size_y, size_z, v_lhs_0_stream);
  //init_aie(size_x, size_y, size_z, v_lhs_1_stream);
  //init_aie(size_x, size_y, size_z, v_mul_0_stream);
  //init_aie(size_x, size_y, size_z, v_mul_1_stream);

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y= get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    stream_to_aie_for_advect_v_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, tzc1, tzc2, tcx, tcy, size_x, chunk_size_y, size_z, v_lhs_0_stream, v_rhs_0_stream,
        v_lhs_1_stream, v_rhs_1_stream, v_mul_0_stream, v_mul_1_stream);
  }
}

static void stream_to_aie_for_advect_v_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& v_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& v_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_1_stream) {
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

        qdma_axis<128,0,0,0> a1, a2, b1, b2;
        ap_uint<128> a1_d, a2_d, b1_d, b2_d;
        float empty=0.0;

        a1.keep_all();
        a2.keep_all();
        b1.keep_all();
        b2.keep_all();

        a1_d.range(31,0)=pack_val(u_stencil.values[0][1][1]);
        b1_d.range(31,0)=pack_val(u_stencil.values[1][2][1]);

        a1_d.range(63,32)=pack_val(u_stencil.values[1][1][1]);
        b1_d.range(63,32)=pack_val(u_stencil.values[1][2][1]);

        a1_d.range(95,64)=pack_val(v_stencil.values[1][1][1]);
        b1_d.range(95,64)=pack_val(v_stencil.values[1][0][1]);

        a1_d.range(127,96)=pack_val(v_stencil.values[1][1][1]);
        b1_d.range(127,96)=pack_val(v_stencil.values[1][2][1]);

        a2_d.range(31,0)=pack_val(w_stencil.values[1][1][0]);
        b2_d.range(31,0)=pack_val(w_stencil.values[1][2][0]);

        a2_d.range(63,32)=pack_val(w_stencil.values[1][1][1]);
        b2_d.range(63,32)=pack_val(w_stencil.values[1][2][1]);

        a2_d.range(95,64)=pack_val(empty);
        b2_d.range(95,64)=pack_val(empty);

        a2_d.range(127,96)=pack_val(empty);
        b2_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        a2.data=a2_d;
        b1.data=b1_d;
        b2.data=b2_d;

        v_lhs_0_stream.write(a1);
        v_lhs_1_stream.write(a2);
        v_rhs_0_stream.write(b1);
        v_rhs_1_stream.write(b2);

        a1_d.range(31,0)=pack_val(v_stencil.values[0][1][1]);
        b1_d.range(31,0)=pack_val(v_stencil.values[1][1][0]);

        a1_d.range(63,32)=pack_val(v_stencil.values[2][1][1]);
        b1_d.range(63,32)=pack_val(v_stencil.values[1][1][2]);

        a1_d.range(95,64)=pack_val(v_stencil.values[1][0][1]);
        b1_d.range(95,64)=pack_val(empty);

        a1_d.range(127,96)=pack_val(v_stencil.values[1][2][1]);
        b1_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        b1.data=b1_d;

        v_mul_0_stream.write(a1);
        v_mul_1_stream.write(b1);
      }
    }
  }
}

void stream_to_aie_for_advect_w(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& w_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& w_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& w_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& w_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& w_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& w_mul_1_stream) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  //init_aie(size_x, size_y, size_z, w_lhs_0_stream);
  //init_aie(size_x, size_y, size_z, w_lhs_1_stream);
  //init_aie(size_x, size_y, size_z, w_mul_0_stream);
  //init_aie(size_x, size_y, size_z, w_mul_1_stream);

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size_y= get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    stream_to_aie_for_advect_w_chunk(u_stencil_stream, v_stencil_stream, w_stencil_stream, tzc1, tzc2, tcx, tcy, size_x, chunk_size_y, size_z, w_lhs_0_stream, w_rhs_0_stream,
        w_lhs_1_stream, w_rhs_1_stream, w_mul_0_stream, w_mul_1_stream);
  }
}

static void stream_to_aie_for_advect_w_chunk(hls::stream<struct stencil_data> & u_stencil_stream, hls::stream<struct stencil_data> & v_stencil_stream, hls::stream<struct stencil_data> & w_stencil_stream,
    REAL_TYPE tzc1[MAX_Z_SIZE], REAL_TYPE tzc2[MAX_Z_SIZE], double tcx, double tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z,
    hls::stream<qdma_axis<128,0,0,0> >& v_lhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_rhs_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_lhs_1_stream,
    hls::stream<qdma_axis<128,0,0,0> >& v_rhs_1_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_0_stream, hls::stream<qdma_axis<128,0,0,0> >& v_mul_1_stream) {
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

        qdma_axis<128,0,0,0> a1, a2, b1, b2;
        ap_uint<128> a1_d, a2_d, b1_d, b2_d;
        float empty=0.0;

        a1.keep_all();
        a2.keep_all();
        b1.keep_all();
        b2.keep_all();

        a1_d.range(31,0)=pack_val(u_stencil.values[1][1][1]);
        b1_d.range(31,0)=pack_val(u_stencil.values[0][1][2]);

        a1_d.range(63,32)=pack_val(u_stencil.values[1][1][1]);
        b1_d.range(63,32)=pack_val(u_stencil.values[1][1][2]);

        a1_d.range(95,64)=pack_val(v_stencil.values[1][0][1]);
        b1_d.range(95,64)=pack_val(v_stencil.values[1][0][2]);

        a1_d.range(127,96)=pack_val(v_stencil.values[1][1][1]);
        b1_d.range(127,96)=pack_val(v_stencil.values[1][1][2]);

        a2_d.range(31,0)=pack_val(w_stencil.values[1][1][1]);
        b2_d.range(31,0)=pack_val(w_stencil.values[1][1][0]);

        a2_d.range(63,32)=pack_val(w_stencil.values[1][1][1]);
        b2_d.range(63,32)=pack_val(w_stencil.values[1][1][2]);

        a2_d.range(95,64)=pack_val(empty);
        b2_d.range(95,64)=pack_val(empty);

        a2_d.range(127,96)=pack_val(empty);
        b2_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        a2.data=a2_d;
        b1.data=b1_d;
        b2.data=b2_d;

        v_lhs_0_stream.write(a1);
        v_lhs_1_stream.write(a2);
        v_rhs_0_stream.write(b1);
        v_rhs_1_stream.write(b2);

        a1_d.range(31,0)=pack_val(w_stencil.values[0][1][1]);
        b1_d.range(31,0)=pack_val(w_stencil.values[1][1][0]);

        a1_d.range(63,32)=pack_val(w_stencil.values[2][1][1]);
        b1_d.range(63,32)=pack_val(w_stencil.values[1][1][2]);

        a1_d.range(95,64)=pack_val(w_stencil.values[1][0][1]);
        b1_d.range(95,64)=pack_val(empty);

        a1_d.range(127,96)=pack_val(w_stencil.values[1][2][1]);
        b1_d.range(127,96)=pack_val(empty);

        a1.data=a1_d;
        b1.data=b1_d;

        v_mul_0_stream.write(a1);
        v_mul_1_stream.write(b1);
      }
    }
  }
}

