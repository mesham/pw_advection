#include "kernel.h"
#include "aie_api/aie.hpp"
#include "aie_api/aie_adf.hpp"
#include <aie_api/utils.hpp>

void cell_advection_fn_addition(input_stream<float> * __restrict in_A, input_stream<float> * __restrict in_B, output_stream<accfloat> * __restrict out) {
  aie::vector<float, 4> in_data;  
  in_data=readincr_v<4>(in_A);  

  int32 its=(int32) in_data.get(0);  

  for (int i=0;i<its;i++) 
  chess_prepare_for_pipelining
  chess_loop_range(64,)
  {
    aie::vector<float,8> lhs_addition_numbers, rhs_addition_numbers;    
    
    lhs_addition_numbers.insert(0,readincr_v<4, aie_stream_resource_in::a>(in_A));
    lhs_addition_numbers.insert(1,readincr_v<4, aie_stream_resource_in::a>(in_A));
      
    rhs_addition_numbers.insert(0,readincr_v<4, aie_stream_resource_in::b>(in_B));
    rhs_addition_numbers.insert(1,readincr_v<4, aie_stream_resource_in::b>(in_B));

    aie::vector<float,8> vadd=aie::add(lhs_addition_numbers, rhs_addition_numbers);
    writeincr_v8(out, vadd);
  }
}

void cell_advection_fn_mul_sub_reduce(input_stream<accfloat> * __restrict in_A, input_stream<float> * __restrict in_B, output_stream<float> * __restrict out) {
  aie::vector<float, 4> in_data;  
  in_data=readincr_v<4>(in_B);  

  int32 its=(int32) in_data.get(0);
  writeincr(out, in_data);

  for (int i=0;i<its;i++) 
  chess_prepare_for_pipelining
  chess_loop_range(64,)
  {
    aie::vector<float,8> lhs_mul_numbers, rhs_mul_numbers;
    
    rhs_mul_numbers.insert(0,readincr_v<4, aie_stream_resource_in::a>(in_B));
    rhs_mul_numbers.insert(1,readincr_v<4, aie_stream_resource_in::b>(in_B));    
    
    lhs_mul_numbers=readincr_v8(in_A);
    
    aie::vector<float,8> vmul=aie::mul(lhs_mul_numbers, rhs_mul_numbers);
    
    aie::vector<float,4> lhs_sub_numbers=vmul.extract<4>(0);
    aie::vector<float,4> rhs_sub_numbers=vmul.extract<4>(1);
    
    aie::vector<float,4> vsub=aie::sub(lhs_sub_numbers, rhs_sub_numbers);
    
    float result=aie::reduce_add(vsub); 
    writeincr<aie_stream_resource_out::a>(out, result);
  }
}
