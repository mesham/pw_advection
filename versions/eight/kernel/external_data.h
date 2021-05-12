#include "definitions.h"
#include <hls_stream.h>
#include <stdio.h>
#include "utils.h"

static void load_y_and_z(struct packaged_double*, hls::stream<REAL_TYPE>&, unsigned int, unsigned int, unsigned int);
static void write_y_and_z(hls::stream<REAL_TYPE>&, struct packaged_double*, unsigned int, unsigned int, unsigned int);

void load_data(struct packaged_double * input, hls::stream<REAL_TYPE> & out_stream, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
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
			load_y_and_z(input, out_stream, start_index, chunk_size, size_z);
		}
	}
}

static void load_y_and_z(struct packaged_double * input, hls::stream<REAL_TYPE> & out_stream, unsigned int start_index, unsigned int chunk_size_y, unsigned int size_z) {
	// First handle the case where start of second row does not divide evenly into external data width
	unsigned int start_point=start_index / EXTERNAL_DATA_WIDTH;

	unsigned int total_retrieve=(chunk_size_y*size_z) / EXTERNAL_DATA_WIDTH;
	unsigned int main_retrieve_part=start_point+total_retrieve;

	unsigned int sp_remainder=EXTERNAL_DATA_WIDTH - (start_index - (start_point * EXTERNAL_DATA_WIDTH));
	if (sp_remainder > 0 && sp_remainder < EXTERNAL_DATA_WIDTH) {
		struct packaged_double element=input[start_point];
		for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) out_stream.write(element.data[j]);
		start_point=start_point+1;
	}

	// Now grab the majority of values in the domain
	read_data_loop:
	for (unsigned int i=start_point;i<main_retrieve_part;i++) {
#pragma HLS PIPELINE II=8
		struct packaged_double element=input[i];
		unpack_loop:
		for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) out_stream.write(element.data[j]);
	}

	// Handle uneven decomposition of last part of data wrt external data width
	unsigned int remainder=(chunk_size_y*size_z) - (total_retrieve*EXTERNAL_DATA_WIDTH) + (EXTERNAL_DATA_WIDTH - sp_remainder);
	if (remainder > 0) {
		struct packaged_double element=input[total_retrieve];
		remainder_loop:
		for (int j=0;j<remainder;j++) out_stream.write(element.data[j]);
	}
}

void write_data(hls::stream<REAL_TYPE> & in_stream, struct packaged_double * output, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
	unsigned int number_chunks=(size_y-4) / (MAX_Y_SIZE - 2);
	unsigned int remainder=(size_y-4) - (number_chunks * (MAX_Y_SIZE - 2));
	if (remainder > 0) number_chunks++;

	chunk_loop:
	for (unsigned int chunk_num=0;chunk_num < number_chunks;chunk_num++) {
		unsigned int chunk_size=get_chunk_size(chunk_num, number_chunks, MAX_Y_SIZE-2, remainder);
		x_read_loop:
		for (unsigned int i=2;i<size_x-2;i++) {
			unsigned int start_index=(i*size_y*size_z) + (((MAX_Y_SIZE - 2) * chunk_num) * size_z);
			write_y_and_z(in_stream, output, start_index, chunk_size, size_z-1);
		}
	}
}

static void write_y_and_z(hls::stream<REAL_TYPE> & in_stream, struct packaged_double * output, unsigned int start_index, unsigned int chunk_size_y, unsigned int size_z) {
	unsigned int start_point=start_index / EXTERNAL_DATA_WIDTH;
	unsigned int sp_remainder=EXTERNAL_DATA_WIDTH - (start_index - (start_point * EXTERNAL_DATA_WIDTH));

	unsigned int total_write=(chunk_size_y*size_z) / EXTERNAL_DATA_WIDTH;
	unsigned int main_retrieve_part=start_point+total_write;

	if (sp_remainder > 0 && sp_remainder < EXTERNAL_DATA_WIDTH) {
		struct packaged_double element;
		for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) element.data[j]=in_stream.read();
		output[start_point]=element;
		start_point=start_point+1;
	}

	write_data_loop:
	for (unsigned int i=start_point;i<main_retrieve_part;i++) {
#pragma HLS PIPELINE II=8
		struct packaged_double element;
		for (int j=0;j<8;j++) element.data[j]=in_stream.read();
		output[i]=element;
	}

	// Now do the remainder elements if doesn't divide evenly
	unsigned int remainder=(chunk_size_y*size_z) - (total_write*EXTERNAL_DATA_WIDTH) + (EXTERNAL_DATA_WIDTH - sp_remainder);
	if (remainder > 0) {
		struct packaged_double element;
		remainder_loop:
		for (int j=0;j<remainder;j++) element.data[j]=in_stream.read();
		output[total_write]=element;
	}
}
