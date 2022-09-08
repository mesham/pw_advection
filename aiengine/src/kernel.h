#ifndef FUNCTION_KERNELS_H
#define FUNCTION_KERNELS_H

#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"

void cell_advection_fn_addition(input_stream<float>* __restrict, input_stream<float>* __restrict, output_window<float>* __restrict);
void cell_advection_fn_mul(input_window<float>* __restrict, input_stream<float>* __restrict, output_window<float>* __restrict);
void cell_advection_fn_sub_reduce(input_window<float>* __restrict, input_window<float>* __restrict, output_stream<float>* __restrict);

#endif
