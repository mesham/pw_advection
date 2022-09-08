#include <adf.h>
#include "kernel.h"

using namespace adf;

class fieldAdvection : public graph {
private:
    kernel cell_advection_fn_addition_1, cell_advection_fn_addition_2, cell_advection_fn_mul_1, cell_advection_fn_mul_2, cell_advection_fn_sub_reduce_1;
public:
    port<input> in_lhs_add_1, in_rhs_add_1, in_lhs_add_2, in_rhs_add_2, in_mul_1, in_mul_2;    
    port<output> out;

    fieldAdvection() {
      cell_advection_fn_addition_1=kernel::create(cell_advection_fn_addition);
      cell_advection_fn_addition_2=kernel::create(cell_advection_fn_addition);
      cell_advection_fn_mul_1=kernel::create(cell_advection_fn_mul);
      cell_advection_fn_mul_2=kernel::create(cell_advection_fn_mul);
      cell_advection_fn_sub_reduce_1=kernel::create(cell_advection_fn_sub_reduce);
      
      connect<stream>(in_lhs_add_1, cell_advection_fn_addition_1.in[0]);
      connect<stream>(in_rhs_add_1, cell_advection_fn_addition_1.in[1]);
      connect<stream>(in_lhs_add_2, cell_advection_fn_addition_2.in[0]);
      connect<stream>(in_rhs_add_2, cell_advection_fn_addition_2.in[1]);
      connect<stream>(in_mul_1, cell_advection_fn_mul_1.in[1]);
      connect<stream>(in_mul_2, cell_advection_fn_mul_2.in[1]);
      connect<window<128> >(cell_advection_fn_addition_1.out[0], cell_advection_fn_mul_1.in[0]);
      connect<window<128> >(cell_advection_fn_addition_2.out[0], cell_advection_fn_mul_2.in[0]);
      connect<window<128> >(cell_advection_fn_mul_1.out[0], cell_advection_fn_sub_reduce_1.in[0]);
      connect<window<128> >(cell_advection_fn_mul_2.out[0], cell_advection_fn_sub_reduce_1.in[1]);
      connect<stream>(cell_advection_fn_sub_reduce_1.out[0], out);
      
      source(cell_advection_fn_addition_1) = "kernel.cpp";
      source(cell_advection_fn_addition_2) = "kernel.cpp";
      source(cell_advection_fn_mul_1) = "kernel.cpp";
      source(cell_advection_fn_mul_2) = "kernel.cpp";
      source(cell_advection_fn_sub_reduce_1) = "kernel.cpp";
      
      runtime<ratio>(cell_advection_fn_addition_1) = 1.0;
      runtime<ratio>(cell_advection_fn_addition_2) = 1.0;
      runtime<ratio>(cell_advection_fn_mul_1) = 1.0;
      runtime<ratio>(cell_advection_fn_mul_2) = 1.0;
      runtime<ratio>(cell_advection_fn_sub_reduce_1) = 1.0;
    }
};

class simpleGraph : public graph {
private:
  fieldAdvection U_advection, V_advection, W_advection;
public:
  input_plio in_U[6], in_V[6], in_W[6];
  output_plio out_U, out_V, out_W;

  simpleGraph(){    

    in_U[0] = input_plio::create("U_add_0_lhs", plio_128_bits, "data/input_A.txt");
    in_U[1] = input_plio::create("U_add_0_rhs", plio_128_bits, "data/input_B.txt");
    in_U[2] = input_plio::create("U_add_1_lhs", plio_128_bits, "data/input_A.txt");
    in_U[3] = input_plio::create("U_add_1_rhs", plio_128_bits, "data/input_B.txt");
    in_U[4] = input_plio::create("U_mul_0", plio_128_bits, "data/input_A.txt");
    in_U[5] = input_plio::create("U_mul_1", plio_128_bits, "data/input_A.txt");
    out_U = output_plio::create("U_out", plio_32_bits, "data/output_U.txt");
    
    connect<stream>(in_U[0].out[0], U_advection.in_lhs_add_1);
    connect<stream>(in_U[1].out[0], U_advection.in_rhs_add_1);
    connect<stream>(in_U[2].out[0], U_advection.in_lhs_add_2);
    connect<stream>(in_U[3].out[0], U_advection.in_rhs_add_2);
    connect<stream>(in_U[4].out[0], U_advection.in_mul_1);
    connect<stream>(in_U[5].out[0], U_advection.in_mul_2);
    connect<stream>(U_advection.out, out_U.in[0]);
    
    in_V[0] = input_plio::create("V_add_0_lhs", plio_128_bits, "data/input_A.txt");
    in_V[1] = input_plio::create("V_add_0_rhs", plio_128_bits, "data/input_B.txt");
    in_V[2] = input_plio::create("V_add_1_lhs", plio_128_bits, "data/input_A.txt");
    in_V[3] = input_plio::create("V_add_1_rhs", plio_128_bits, "data/input_B.txt");
    in_V[4] = input_plio::create("V_mul_0", plio_128_bits, "data/input_A.txt");
    in_V[5] = input_plio::create("V_mul_1", plio_128_bits, "data/input_A.txt");
    out_V = output_plio::create("V_out", plio_32_bits, "data/output_V.txt");
    
    connect<stream>(in_V[0].out[0], V_advection.in_lhs_add_1);
    connect<stream>(in_V[1].out[0], V_advection.in_rhs_add_1);
    connect<stream>(in_V[2].out[0], V_advection.in_lhs_add_2);
    connect<stream>(in_V[3].out[0], V_advection.in_rhs_add_2);
    connect<stream>(in_V[4].out[0], V_advection.in_mul_1);
    connect<stream>(in_V[5].out[0], V_advection.in_mul_2);
    connect<stream>(V_advection.out, out_V.in[0]);
      
    in_W[0] = input_plio::create("W_add_0_lhs", plio_128_bits, "data/input_A.txt");
    in_W[1] = input_plio::create("W_add_0_rhs", plio_128_bits, "data/input_B.txt");
    in_W[2] = input_plio::create("W_add_1_lhs", plio_128_bits, "data/input_A.txt");
    in_W[3] = input_plio::create("W_add_1_rhs", plio_128_bits, "data/input_B.txt");
    in_W[4] = input_plio::create("W_mul_0", plio_128_bits, "data/input_A.txt");
    in_W[5] = input_plio::create("W_mul_1", plio_128_bits, "data/input_A.txt");
    out_W = output_plio::create("W_out", plio_32_bits, "data/output_W.txt");
    
    connect<stream>(in_W[0].out[0], W_advection.in_lhs_add_1);
    connect<stream>(in_W[1].out[0], W_advection.in_rhs_add_1);
    connect<stream>(in_W[2].out[0], W_advection.in_lhs_add_2);
    connect<stream>(in_W[3].out[0], W_advection.in_rhs_add_2);
    connect<stream>(in_W[4].out[0], W_advection.in_mul_1);
    connect<stream>(in_W[5].out[0], W_advection.in_mul_2);
    connect<stream>(W_advection.out, out_W.in[0]);    
  }
};
