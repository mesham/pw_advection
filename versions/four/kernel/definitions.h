#ifndef DEFINITIONS_HEADER
#define DEFINITIONS_HEADER

#define MAX_Z_SIZE 64
#define MAX_Y_SIZE 260
#define REAL_TYPE double

struct stencil_data {
	REAL_TYPE values[3][3][3];
};

#define EXTERNAL_DATA_WIDTH 8

struct packaged_double {
	double data[EXTERNAL_DATA_WIDTH];
};

#endif
