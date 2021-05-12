#include "definitions.h"
#include <hls_stream.h>

void load_data(REAL_TYPE * input, hls::stream<REAL_TYPE> & out_stream, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
	unsigned total_retrieve=(size_x-1)*size_y*size_z;
	read_data_loop:
	for (unsigned int i=size_y*size_z;i<total_retrieve;i++) {
		REAL_TYPE data=input[i];
		out_stream.write(data);
	}
}

void write_data(hls::stream<REAL_TYPE> & in_stream, REAL_TYPE * output, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
	unsigned total_write=(size_x-4)*(size_y-4)*(size_z-1);
	write_data_loop:
	for (unsigned int i=0;i<total_write;i++) {
		REAL_TYPE data=in_stream.read();
		output[i]=data;
	}
}

