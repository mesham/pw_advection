#include "definitions.h"
#include "external_data.h"
#include "shift_buffer.h"
#include "advect.h"
#include "utils.h"
#include <hls_stream.h>

static void advect_flow_fields(struct packaged_double*, struct packaged_double*, struct packaged_double*, struct packaged_double*, struct packaged_double*, struct packaged_double*,
		REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE],
		REAL_TYPE, REAL_TYPE, unsigned int, unsigned int, unsigned int);
static void load_constants(REAL_TYPE*, REAL_TYPE*, REAL_TYPE*, REAL_TYPE*, REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE],
		REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], REAL_TYPE[MAX_Z_SIZE], unsigned int);

extern "C" void pw_advection(struct packaged_double * u, struct packaged_double * v, struct packaged_double * w, struct packaged_double * su, struct packaged_double * sv, struct packaged_double * sw,
		REAL_TYPE * tzc1, REAL_TYPE * tzc2, REAL_TYPE * tzd1, REAL_TYPE * tzd2, REAL_TYPE tcx, REAL_TYPE tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {
#pragma HLS INTERFACE m_axi port=su offset=slave bundle=su_port
#pragma HLS INTERFACE m_axi port=sv offset=slave bundle=sv_port
#pragma HLS INTERFACE m_axi port=sw offset=slave bundle=sw_port
#pragma HLS INTERFACE m_axi port=u offset=slave bundle=u_port
#pragma HLS INTERFACE m_axi port=v offset=slave bundle=v_port
#pragma HLS INTERFACE m_axi port=w offset=slave bundle=w_port
#pragma HLS INTERFACE m_axi port=tzc1 offset=slave bundle=constants_port
#pragma HLS INTERFACE m_axi port=tzc2 offset=slave bundle=constants_port
#pragma HLS INTERFACE m_axi port=tzd1 offset=slave bundle=constants_port
#pragma HLS INTERFACE m_axi port=tzd2 offset=slave bundle=constants_port

#pragma HLS INTERFACE s_axilite port=su bundle=control
#pragma HLS INTERFACE s_axilite port=sv bundle=control
#pragma HLS INTERFACE s_axilite port=sw bundle=control
#pragma HLS INTERFACE s_axilite port=u bundle=control
#pragma HLS INTERFACE s_axilite port=v bundle=control
#pragma HLS INTERFACE s_axilite port=w bundle=control
#pragma HLS INTERFACE s_axilite port=tzc1 bundle=control
#pragma HLS INTERFACE s_axilite port=tzc2 bundle=control
#pragma HLS INTERFACE s_axilite port=tzd1 bundle=control
#pragma HLS INTERFACE s_axilite port=tzd2 bundle=control
#pragma HLS INTERFACE s_axilite port=tcx bundle=control
#pragma HLS INTERFACE s_axilite port=tcy bundle=control
#pragma HLS INTERFACE s_axilite port=size_x bundle=control
#pragma HLS INTERFACE s_axilite port=size_y bundle=control
#pragma HLS INTERFACE s_axilite port=size_z bundle=control
#pragma HLS INTERFACE s_axilite port=return bundle=control

	REAL_TYPE tzc1_local_1[MAX_Z_SIZE], tzc2_local_1[MAX_Z_SIZE], tzc1_local_2[MAX_Z_SIZE], tzc2_local_2[MAX_Z_SIZE], tzd1_local[MAX_Z_SIZE], tzd2_local[MAX_Z_SIZE];

	load_constants(tzc1, tzc2, tzd1, tzd2, tzc1_local_1, tzc2_local_1, tzc1_local_2, tzc2_local_2, tzd1_local, tzd2_local, size_z);
	advect_flow_fields(u, v, w, su, sv, sw, tzc1_local_1, tzc2_local_1, tzc1_local_2, tzc2_local_2, tzd1_local, tzd2_local, tcx, tcy, size_x, size_y, size_z);
}

static void advect_flow_fields(struct packaged_double * u, struct packaged_double * v, struct packaged_double * w,
		struct packaged_double * su, struct packaged_double * sv, struct packaged_double * sw,
		REAL_TYPE tzc1_local_1[MAX_Z_SIZE], REAL_TYPE tzc2_local_1 [MAX_Z_SIZE], REAL_TYPE tzc1_local_2[MAX_Z_SIZE], REAL_TYPE tzc2_local_2[MAX_Z_SIZE],
		REAL_TYPE tzd1_local[MAX_Z_SIZE], REAL_TYPE tzd2_local[MAX_Z_SIZE],
		REAL_TYPE tcx, REAL_TYPE tcy, unsigned int size_x, unsigned int size_y, unsigned int size_z) {

	hls::stream<REAL_TYPE> u_stream("u_stream"), v_stream("v_stream"), w_stream("w_stream"), su_stream("su_stream"), sv_stream("sv_stream"), sw_stream("sw_stream");
	hls::stream<struct stencil_data> stencil_u_stream("stencil_u_stream"), stencil_v_stream("stencil_v_stream"), stencil_w_stream("stencil_w_stream");
	hls::stream<struct stencil_data> stencil_u_stream_dup[3], stencil_v_stream_dup[3], stencil_w_stream_dup[3];

#pragma HLS STREAM variable=u_stream depth=16
#pragma HLS STREAM variable=v_stream depth=16
#pragma HLS STREAM variable=w_stream depth=16
#pragma HLS STREAM variable=su_stream depth=16
#pragma HLS STREAM variable=sv_stream depth=16
#pragma HLS STREAM variable=sw_stream depth=16
#pragma HLS STREAM variable=stencil_u_stream depth=16
#pragma HLS STREAM variable=stencil_v_stream depth=16
#pragma HLS STREAM variable=stencil_w_stream depth=16
#pragma HLS STREAM variable=stencil_u_stream_dup depth=16
#pragma HLS STREAM variable=stencil_v_stream_dup depth=16
#pragma HLS STREAM variable=stencil_w_stream_dup depth=16

#pragma HLS DATAFLOW
	load_data(u, u_stream, size_x, size_y, size_z);
	load_data(v, v_stream, size_x, size_y, size_z);
	load_data(w, w_stream, size_x, size_y, size_z);

	shift_buffer(u_stream, stencil_u_stream, size_x-2, size_y, size_z);
	shift_buffer(v_stream, stencil_v_stream, size_x-2, size_y, size_z);
	shift_buffer(w_stream, stencil_w_stream, size_x-2, size_y, size_z);

	duplicateStream<struct stencil_data, 3>(stencil_u_stream, stencil_u_stream_dup, (size_x-4)*(size_y-4)*(size_z-1));
	duplicateStream<struct stencil_data, 3>(stencil_v_stream, stencil_v_stream_dup, (size_x-4)*(size_y-4)*(size_z-1));
	duplicateStream<struct stencil_data, 3>(stencil_w_stream, stencil_w_stream_dup, (size_x-4)*(size_y-4)*(size_z-1));

	advect_u(stencil_u_stream_dup[0], stencil_v_stream_dup[0], stencil_w_stream_dup[0], su_stream, tzc1_local_1, tzc2_local_1, tcx, tcy, size_x, size_y, size_z);
	advect_v(stencil_u_stream_dup[1], stencil_v_stream_dup[1], stencil_w_stream_dup[1], sv_stream, tzc1_local_2, tzc2_local_2, tcx, tcy, size_x, size_y, size_z);
	advect_w(stencil_u_stream_dup[2], stencil_v_stream_dup[2], stencil_w_stream_dup[2], sw_stream, tzd1_local, tzd2_local, tcx, tcy, size_x, size_y, size_z);

	write_data(su_stream, su, size_x, size_y, size_z);
	write_data(sv_stream, sv, size_x, size_y, size_z);
	write_data(sw_stream, sw, size_x, size_y, size_z);
}

static void load_constants(REAL_TYPE * tzc1, REAL_TYPE * tzc2, REAL_TYPE * tzd1, REAL_TYPE * tzd2, REAL_TYPE tzc1_local_1[MAX_Z_SIZE], REAL_TYPE tzc2_local_1[MAX_Z_SIZE],
		REAL_TYPE tzc1_local_2[MAX_Z_SIZE], REAL_TYPE tzc2_local_2[MAX_Z_SIZE], REAL_TYPE tzd1_local[MAX_Z_SIZE], REAL_TYPE tzd2_local[MAX_Z_SIZE], unsigned int size_z) {
	for (unsigned int i=0;i<size_z;i++) {
#pragma HLS PIPELINE II=4
		tzc1_local_1[i]=tzc1[i];
		tzc2_local_1[i]=tzc2[i];
		tzc1_local_2[i]=tzc1[i];
		tzc2_local_2[i]=tzc2[i];
		tzd1_local[i]=tzd1[i];
		tzd2_local[i]=tzd2[i];
	}
}
