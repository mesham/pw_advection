#include <adf.h>
#include "kernel.h"

using namespace adf;

class fieldAdvection : public graph {
private:
    kernel kernel_cell_advection_fn_addition, kernel_cell_advection_fn_mul_sub_reduce;
public:
    port<input> in_lhs_add_1, in_rhs_add_1, in_mul_1;
    port<output> out;

    fieldAdvection() {
      kernel_cell_advection_fn_addition=kernel::create(cell_advection_fn_addition);
      kernel_cell_advection_fn_mul_sub_reduce=kernel::create(cell_advection_fn_mul_sub_reduce);      
      
      connect<>(in_lhs_add_1, kernel_cell_advection_fn_addition.in[0]);
      connect<>(in_rhs_add_1, kernel_cell_advection_fn_addition.in[1]);      
      connect<>(in_mul_1, kernel_cell_advection_fn_mul_sub_reduce.in[1]);      
      connect<cascade>(kernel_cell_advection_fn_addition.out[0], kernel_cell_advection_fn_mul_sub_reduce.in[0]);      
      connect<>(kernel_cell_advection_fn_mul_sub_reduce.out[0], out);
      
      source(kernel_cell_advection_fn_addition) = "kernel.cpp";
      source(kernel_cell_advection_fn_mul_sub_reduce) = "kernel.cpp";      
      
      runtime<ratio>(kernel_cell_advection_fn_addition) = 1.0;
      runtime<ratio>(kernel_cell_advection_fn_mul_sub_reduce) = 1.0;      
    }
};

class simpleGraph : public graph {
private:
  fieldAdvection U_advection, V_advection, W_advection;
public:
  input_plio in_U[3], in_V[3], in_W[3];
  output_plio out_U, out_V, out_W;

  simpleGraph(){    

    in_U[0] = input_plio::create("U_add_lhs", plio_128_bits, "data/input_A.txt");
    in_U[1] = input_plio::create("U_add_rhs", plio_128_bits, "data/input_B.txt");    
    in_U[2] = input_plio::create("U_mul", plio_128_bits, "data/input_A.txt");    
    out_U = output_plio::create("U_out", plio_32_bits, "data/output_U.txt");
    
    connect<stream>(in_U[0].out[0], U_advection.in_lhs_add_1);
    connect<stream>(in_U[1].out[0], U_advection.in_rhs_add_1);
    connect<stream>(in_U[2].out[0], U_advection.in_mul_1);    
    connect<stream>(U_advection.out, out_U.in[0]);
    
    in_V[0] = input_plio::create("V_add_lhs", plio_128_bits, "data/input_A.txt");
    in_V[1] = input_plio::create("V_add_rhs", plio_128_bits, "data/input_B.txt");    
    in_V[2] = input_plio::create("V_mul", plio_128_bits, "data/input_A.txt");
    out_V = output_plio::create("V_out", plio_32_bits, "data/output_V.txt");
    
    connect<stream>(in_V[0].out[0], V_advection.in_lhs_add_1);
    connect<stream>(in_V[1].out[0], V_advection.in_rhs_add_1);
    connect<stream>(in_V[2].out[0], V_advection.in_mul_1);    
    connect<stream>(V_advection.out, out_V.in[0]);
      
    in_W[0] = input_plio::create("W_add_lhs", plio_128_bits, "data/input_A.txt");
    in_W[1] = input_plio::create("W_add_rhs", plio_128_bits, "data/input_B.txt");    
    in_W[2] = input_plio::create("W_mul", plio_128_bits, "data/input_A.txt");
    out_W = output_plio::create("W_out", plio_32_bits, "data/output_W.txt");
    
    connect<stream>(in_W[0].out[0], W_advection.in_lhs_add_1);
    connect<stream>(in_W[1].out[0], W_advection.in_rhs_add_1);
    connect<stream>(in_W[2].out[0], W_advection.in_mul_1);    
    connect<stream>(W_advection.out, out_W.in[0]);    
  }
};
