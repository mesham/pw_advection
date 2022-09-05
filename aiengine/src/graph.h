#include <adf.h>
#include "kernel.h"

using namespace adf;

class simpleGraph : public graph {
private:
  kernel cell_advection_kernel[3];
public:
  input_plio in_A[3], in_B[3];
  output_plio out[3];

  simpleGraph(){
    cell_advection_kernel[0] = kernel::create(cell_advection_fn);
    cell_advection_kernel[1] = kernel::create(cell_advection_fn);
    cell_advection_kernel[2] = kernel::create(cell_advection_fn);

    in_A[0] = input_plio::create("krnl_0_in0", plio_128_bits, "data/input_A.txt");
    in_B[0]  = input_plio::create("krnl_0_in1", plio_128_bits, "data/input_B.txt");
    out[0] = output_plio::create("krnl_0_out1", plio_32_bits, "data/output_0.txt");

    in_A[1] = input_plio::create("krnl_1_in0", plio_128_bits, "data/input_A.txt");
    in_B[1]  = input_plio::create("krnl_1_in1", plio_128_bits, "data/input_B.txt");
    out[1] = output_plio::create("krnl_1_out1", plio_32_bits, "data/output_1.txt");

    in_A[2] = input_plio::create("krnl_2_in0", plio_128_bits, "data/input_A.txt");
    in_B[2]  = input_plio::create("krnl_2_in1", plio_128_bits, "data/input_B.txt");
    out[2] = output_plio::create("krnl_2_out1", plio_32_bits, "data/output_2.txt");

    for (int i=0;i<3;i++) {
      connect<stream>(in_A[i].out[0], cell_advection_kernel[i].in[0]);
      connect<stream>(in_B[i].out[0], cell_advection_kernel[i].in[1]);
      connect<stream>(cell_advection_kernel[i].out[0], out[i].in[0]);

      source(cell_advection_kernel[i]) = "kernel.cpp";
      runtime<ratio>(cell_advection_kernel[i]) = 1.0;
    }    
  }
};
