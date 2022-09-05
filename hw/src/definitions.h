#ifndef DEFINITIONS_HEADER
#define DEFINITIONS_HEADER

#define MAX_Z_SIZE 64
#define MAX_Y_SIZE 16
#define REAL_TYPE float

struct stencil_data {
  REAL_TYPE values[3][3][3];
};

#define EXTERNAL_DATA_WIDTH 16

struct packaged_double {
  REAL_TYPE data[EXTERNAL_DATA_WIDTH];
};

#endif
