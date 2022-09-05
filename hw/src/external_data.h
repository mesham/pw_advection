#include "definitions.h"
#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_int.h>
#include <stdio.h>
#include "utils.h"

const int load_write_pipeline_ii=EXTERNAL_DATA_WIDTH;

static void load_y_and_z(struct packaged_double*, struct packaged_double*, struct packaged_double*, hls::stream<REAL_TYPE>&,
    hls::stream<REAL_TYPE>&,hls::stream<REAL_TYPE>&, unsigned int, unsigned int, unsigned int);
static void write_y_and_z(hls::stream<qdma_axis<32,0,0,0> > &, hls::stream<qdma_axis<32,0,0,0> > &, hls::stream<qdma_axis<32,0,0,0> > &, struct packaged_double*,
    struct packaged_double*, struct packaged_double*, unsigned int, unsigned int, unsigned int);

void load_data(struct packaged_double * input_u, struct packaged_double * input_v, struct packaged_double * input_w, hls::stream<REAL_TYPE> & out_u_stream,
    hls::stream<REAL_TYPE> & out_v_stream, hls::stream<REAL_TYPE> & out_w_stream, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  unsigned int number_access_y=get_number_y_access_with_overlap(size_y);
  unsigned int number_chunks=number_access_y / MAX_Y_SIZE;
  unsigned int remainder=number_access_y - (number_chunks * MAX_Y_SIZE);
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size=get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE, remainder);
    unsigned int chunk_start_y;
    if (chunk_num == 0) {
      chunk_start_y=1;
    } else {
      chunk_start_y=(chunk_num*MAX_Y_SIZE) - (2 * chunk_num);
    }

    x_read_loop:
    for (unsigned int i=1;i<size_x-1;i++) {
      unsigned int start_index=(i*size_y*size_z) + chunk_start_y;
      load_y_and_z(input_u, input_v, input_w, out_u_stream, out_v_stream, out_w_stream, start_index, chunk_size, size_z);
    }
  }
}

static void load_y_and_z(struct packaged_double * input_u, struct packaged_double * input_v, struct packaged_double * input_w, hls::stream<REAL_TYPE> & out_u_stream,
    hls::stream<REAL_TYPE> & out_v_stream, hls::stream<REAL_TYPE> & out_w_stream, unsigned int start_index, unsigned int chunk_size_y, unsigned int size_z) {
  // First handle the case where start of second row does not divide evenly into external data width
  unsigned int start_point=start_index / EXTERNAL_DATA_WIDTH;

  unsigned int total_retrieve=(chunk_size_y*size_z) / EXTERNAL_DATA_WIDTH;
  unsigned int main_retrieve_part=start_point+total_retrieve;

  unsigned int sp_remainder=EXTERNAL_DATA_WIDTH - (start_index - (start_point * EXTERNAL_DATA_WIDTH));
  if (sp_remainder > 0 && sp_remainder < EXTERNAL_DATA_WIDTH) {
    struct packaged_double element_u=input_u[start_point];
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) out_u_stream.write(element_u.data[j]);

    struct packaged_double element_v=input_v[start_point];
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) out_v_stream.write(element_v.data[j]);

    struct packaged_double element_w=input_w[start_point];
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) out_w_stream.write(element_w.data[j]);
    start_point=start_point+1;
  }

  // Now grab the majority of values in the domain
  read_data_loop:
  for (unsigned int i=start_point;i<main_retrieve_part;i++) {
#pragma HLS PIPELINE II=load_write_pipeline_ii
    struct packaged_double element_u=input_u[i];
    unpack_u_loop:
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) out_u_stream.write(element_u.data[j]);

    struct packaged_double element_v=input_v[i];
    unpack_v_loop:
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) out_v_stream.write(element_v.data[j]);

    struct packaged_double element_w=input_w[i];
    unpack_w_loop:
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) out_w_stream.write(element_w.data[j]);
  }

  // Handle uneven decomposition of last part of data wrt external data width
  unsigned int remainder=(chunk_size_y*size_z) - (total_retrieve*EXTERNAL_DATA_WIDTH) + (EXTERNAL_DATA_WIDTH - sp_remainder);
  if (remainder > 0) {
    struct packaged_double element_u=input_u[total_retrieve];
    remainder_u_loop:
    for (int j=0;j<remainder;j++) out_u_stream.write(element_u.data[j]);

    struct packaged_double element_v=input_v[total_retrieve];
    remainder_v_loop:
    for (int j=0;j<remainder;j++) out_v_stream.write(element_v.data[j]);

    struct packaged_double element_w=input_w[total_retrieve];
    remainder_w_loop:
    for (int j=0;j<remainder;j++) out_w_stream.write(element_w.data[j]);
  }
}

void write_data(hls::stream<qdma_axis<32,0,0,0> > & su_result_stream, hls::stream<qdma_axis<32,0,0,0> > & sv_result_stream, hls::stream<qdma_axis<32,0,0,0> > & sw_result_stream,
    struct packaged_double * output_su, struct packaged_double * output_sv, struct packaged_double * output_sw, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
  unsigned int number_chunks=(size_y-4) / (MAX_Y_SIZE - 2);
  unsigned int remainder=(size_y-4) - (number_chunks * (MAX_Y_SIZE - 2));
  if (remainder > 0) number_chunks++;

  chunk_loop:
  for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
    unsigned int chunk_size=get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE-2, remainder);
    x_read_loop:
    for (unsigned int i=2;i<size_x-2;i++) {
      unsigned int start_index=(i*size_y*size_z) + (((MAX_Y_SIZE - 2) * chunk_num) * size_z);
      write_y_and_z(su_result_stream, sv_result_stream, sw_result_stream, output_su, output_sv, output_sw, start_index, chunk_size, size_z-1);
    }
  }
}

static REAL_TYPE get_float(qdma_axis<32,0,0,0> d) {
  ap_uint<32> data=d.data;
  return *(REAL_TYPE*)(&data);
}

static void write_y_and_z(hls::stream<qdma_axis<32,0,0,0> > & su_result_stream, hls::stream<qdma_axis<32,0,0,0> > & sv_result_stream, hls::stream<qdma_axis<32,0,0,0> > & sw_result_stream,
    struct packaged_double * output_su, struct packaged_double * output_sv, struct packaged_double * output_sw, unsigned int start_index, unsigned int chunk_size_y, unsigned int size_z) {
  unsigned int start_point=start_index / EXTERNAL_DATA_WIDTH;
  unsigned int sp_remainder=EXTERNAL_DATA_WIDTH - (start_index - (start_point * EXTERNAL_DATA_WIDTH));

  unsigned int total_write=(chunk_size_y*size_z) / EXTERNAL_DATA_WIDTH;
  unsigned int main_retrieve_part=start_point+total_write;

  if (sp_remainder > 0 && sp_remainder < EXTERNAL_DATA_WIDTH) {
    struct packaged_double element_su;
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) element_su.data[j]=get_float(su_result_stream.read());
    output_su[start_point]=element_su;

    struct packaged_double element_sv;
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) element_sv.data[j]=get_float(sv_result_stream.read());
    output_sv[start_point]=element_sv;

    struct packaged_double element_sw;
    for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) element_sw.data[j]=get_float(sw_result_stream.read());
    output_sw[start_point]=element_sw;
    start_point=start_point+1;
  }

  write_data_loop:
  for (unsigned int i=start_point;i<main_retrieve_part;i++) {
#pragma HLS PIPELINE II=load_write_pipeline_ii
    struct packaged_double element_su;
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) element_su.data[j]=get_float(su_result_stream.read());
    output_su[i]=element_su;

    struct packaged_double element_sv;
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) element_sv.data[j]=get_float(sv_result_stream.read());
    output_sv[i]=element_sv;

    struct packaged_double element_sw;
    for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) element_sw.data[j]=get_float(sw_result_stream.read());
    output_sw[i]=element_sw;
  }

  // Now do the remainder elements if doesn't divide evenly
  unsigned int remainder=(chunk_size_y*size_z) - (total_write*EXTERNAL_DATA_WIDTH) + (EXTERNAL_DATA_WIDTH - sp_remainder);
  if (remainder > 0) {
    struct packaged_double element_su;
    remainder_u_loop:
    for (int j=0;j<remainder;j++) element_su.data[j]=get_float(su_result_stream.read());
    output_su[total_write]=element_su;

    struct packaged_double element_sv;
    remainder_v_loop:
    for (int j=0;j<remainder;j++) element_sv.data[j]=get_float(sv_result_stream.read());
    output_sv[total_write]=element_sv;

    struct packaged_double element_sw;
    remainder_w_loop:
    for (int j=0;j<remainder;j++) element_sw.data[j]=get_float(sw_result_stream.read());
    output_sw[total_write]=element_sw;
  }
}
