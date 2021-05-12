#include "definitions.h"
#include <hls_stream.h>
#include <stdio.h>

void load_data(struct packaged_double * input, hls::stream<REAL_TYPE> & out_stream, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
	// First handle the case where start of second row does not divide evenly into external data width
	unsigned int start_point=(size_y*size_z) / EXTERNAL_DATA_WIDTH;
	unsigned int sp_remainder=(size_y*size_z) - (start_point * EXTERNAL_DATA_WIDTH);
	if (sp_remainder > 0) {
		struct packaged_double element=input[start_point];
		for (int j=EXTERNAL_DATA_WIDTH-sp_remainder;j<EXTERNAL_DATA_WIDTH;j++) out_stream.write(element.data[j]);
		start_point=start_point+1;
	}

	// Now grab the majority of values in the domain
	unsigned int total_retrieve=((size_x-1)*size_y*size_z) / EXTERNAL_DATA_WIDTH;
	read_data_loop:
	for (unsigned int i=start_point;i<total_retrieve;i++) {
#pragma HLS PIPELINE II=8
		struct packaged_double element=input[i];
		unpack_loop:
		for (int j=0;j<EXTERNAL_DATA_WIDTH;j++) out_stream.write(element.data[j]);
	}

	// Handle uneven decomposition of last part of data wrt external data width
	unsigned int remainder=((size_x-1)*size_y*size_z) - (total_retrieve*EXTERNAL_DATA_WIDTH);
	if (remainder > 0) {
		struct packaged_double element=input[total_retrieve];
		remainder_loop:
		for (int j=0;j<remainder;j++) out_stream.write(element.data[j]);
	}
}

void write_data(hls::stream<REAL_TYPE> & in_stream, struct packaged_double * output, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
	unsigned int total_write=((size_x-4)*(size_y-4)*(size_z-1)) / EXTERNAL_DATA_WIDTH;
	write_data_loop:
	for (unsigned int i=0;i<total_write;i++) {
#pragma HLS PIPELINE II=8
		struct packaged_double element;
		for (int j=0;j<8;j++) element.data[j]=in_stream.read();
		output[i]=element;
	}
	// Now do the remainder elements if doesn't divide evenly
	unsigned int remainder=((size_x-4)*(size_y-4)*(size_z-1)) - (total_write*EXTERNAL_DATA_WIDTH);
	if (remainder > 0) {
		struct packaged_double element;
		remainder_loop:
		for (int j=0;j<remainder;j++) element.data[j]=in_stream.read();
		output[total_write]=element;
	}
}
