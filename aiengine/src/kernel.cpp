#include "kernel.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include <aie_api/utils.hpp>

void cell_advection_fn_addition(input_stream<float> * __restrict in_A, input_stream<float> * __restrict in_B, output_window<float> * __restrict out) {
    aie::vector<float,4> lhs_addition_numbers=readincr_v<4, aie_stream_resource_in::a>(in_A);
    aie::vector<float,4> rhs_addition_numbers=readincr_v<4, aie_stream_resource_in::b>(in_B);

    aie::vector<float,4> vadd=aie::add(lhs_addition_numbers, rhs_addition_numbers);
    window_writeincr(out, vadd);
}

void cell_advection_fn_mul(input_window<float> * __restrict in_A, input_stream<float> * __restrict in_B, output_window<float> * __restrict out) {
    aie::vector<float,4> lhs_mul_numbers=window_readincr_v<4>(in_A);
    aie::vector<float,4> rhs_mul_numbers=readincr_v<4, aie_stream_resource_in::b>(in_B);
    
    aie::vector<float,4> vmul=aie::mul(lhs_mul_numbers, rhs_mul_numbers);
    window_writeincr(out, vmul);
}

void cell_advection_fn_sub_reduce(input_window<float> * __restrict in_A, input_window<float> * __restrict in_B, output_stream<float> * __restrict out) {
    aie::vector<float,4> lhs_sub_numbers=window_readincr_v<4>(in_A);
    aie::vector<float,4> rhs_sub_numbers=window_readincr_v<4>(in_B);
    
    aie::vector<float,4> vsub=aie::sub(lhs_sub_numbers, rhs_sub_numbers);
    float result=aie::reduce_add(vsub);
    
    writeincr<aie_stream_resource_out::a>(out, result);
}
