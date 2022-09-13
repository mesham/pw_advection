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
    aie::vector<float,4> lhs_addition_numbers_1, rhs_addition_numbers_1, lhs_addition_numbers_2, rhs_addition_numbers_2;
    
    lhs_addition_numbers_1=readincr_v<4, aie_stream_resource_in::a>(in_A);
    rhs_addition_numbers_1=readincr_v<4, aie_stream_resource_in::b>(in_B);
    aie::vector<float,4> vadd=aie::add(lhs_addition_numbers_1, rhs_addition_numbers_1);
    writeincr_v8(out, vadd.grow<8>(0));
        
    lhs_addition_numbers_2=readincr_v<4, aie_stream_resource_in::a>(in_A);
    rhs_addition_numbers_2=readincr_v<4, aie_stream_resource_in::b>(in_B);    

    aie::vector<float,4> vadd2=aie::add(lhs_addition_numbers_2, rhs_addition_numbers_2);
    writeincr_v8(out, vadd2.grow<8>(0));
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
    aie::vector<float,4> lhs_mul_numbers_1, rhs_mul_numbers_1, lhs_mul_numbers_2, rhs_mul_numbers_2;
    
    rhs_mul_numbers_1=readincr_v<4, aie_stream_resource_in::a>(in_B);            
    aie::vector<float,8> in1=readincr_v8(in_A);
    lhs_mul_numbers_1=in1.extract<4>(0);
    aie::vector<float,4> vmul_1=aie::mul(lhs_mul_numbers_1, rhs_mul_numbers_1);
    
    rhs_mul_numbers_2=readincr_v<4, aie_stream_resource_in::a>(in_B);
    aie::vector<float,8> in2=readincr_v8(in_A);
    lhs_mul_numbers_2=in2.extract<4>(0);
    aie::vector<float,4> vmul_2=aie::mul(lhs_mul_numbers_2, rhs_mul_numbers_2);

    aie::vector<float,4> vsub=aie::sub(vmul_1, vmul_2);
    float result=aie::reduce_add(vsub);
    
    writeincr<aie_stream_resource_out::a>(out, result);
  }
}
