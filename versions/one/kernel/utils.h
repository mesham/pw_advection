#include <hls_stream.h>

#define PRAGMA_SUB(x) _Pragma (#x)
#define PRAGMA(x) PRAGMA_SUB(x)

template <class T, int N>
void duplicateStream(hls::stream<T> & source, hls::stream<T> target[N], unsigned int number) {
	number_loop:
	for (unsigned int i=0;i<number;i++) {
#pragma HLS PIPELINE II=1
		T data=source.read();
		duplicates_loop:
		for (unsigned int j=0;j<N;j++) {
#pragma HLS UNROLL
			target[j].write(data);
		}
	}
}
