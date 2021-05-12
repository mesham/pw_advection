#include "host.hpp"
#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <CL/cl2.hpp>
#include <CL/cl_ext_xilinx.h>

#define X_SIZE 2048
#define Y_SIZE 2048
#define Z_SIZE 64

//#define X_SIZE 8
//#define Y_SIZE 73
//#define Z_SIZE 8

typedef double REAL_TYPE;

#define PAGESIZE 4096

int irlength, hazardlength, n_size;

cl::CommandQueue * command_queue;
cl::Context * context;
cl::Program * program;
cl::Kernel * pw_advection_kernel;
cl::Buffer *u_buffer, *v_buffer, *w_buffer, *su_buffer, *sv_buffer, *sw_buffer, *tzc1_buffer, *tzc2_buffer, *tzd1_buffer, *tzd2_buffer;

REAL_TYPE *u_data, *v_data, *w_data, *su_data, *sv_data, *sw_data, *tzc1_data, *tzc2_data, *tzd1_data, *tzd2_data;
REAL_TYPE tcx, tcy;

int size_in_x=X_SIZE+4, size_in_y=Y_SIZE+4, size_in_z=Z_SIZE;

static void init_device(char*);
static void execute_entire_kernel_on_device(cl::Event&, cl::Event&, cl::Event&);
static float getTimeOfComponent(cl::Event&);
static void initiateData();
static long long getTotalFLOPS();

int main(int argc, char * argv[]) {      
  cl::Event copyOnEvent, kernelExecutionEvent, copyOffEvent;

  printf("Advecting with compute domain X=%d Y=%d Z=%d, total domain size of X=%d Y=%d Z=%d\n", X_SIZE, Y_SIZE, Z_SIZE, size_in_x, size_in_y, size_in_z);

  initiateData();
  init_device(argv[1]);

  execute_entire_kernel_on_device(copyOnEvent, kernelExecutionEvent, copyOffEvent);
#ifdef DISPLAYRESULTS  
  display_results();
#endif

  float copyOnTime=getTimeOfComponent(copyOnEvent);  
  float kernelTime=getTimeOfComponent(kernelExecutionEvent);  
  float copyOffTime=getTimeOfComponent(copyOffEvent);
  
  printf("Execution complete, total runtime : %.3f ms, (%.3f ms xfer on, %.3f ms execute, %.3f ms xfer off)\n", copyOnTime+kernelTime+copyOffTime, copyOnTime, kernelTime, copyOffTime);  

  double totalFLOPS=(getTotalFLOPS() / ((copyOnTime+kernelTime+copyOffTime) /1000)) / 1024 / 1024 / 1024;
  double kernelFLOPS=(getTotalFLOPS() / (kernelTime / 1000)) / 1024 / 1024 / 1024;
  printf("Overall GFLOPS %.2f, kernel GFLOPS %.2f\n", totalFLOPS, kernelFLOPS);

  delete u_buffer;
  delete v_buffer;
  delete w_buffer;
  delete su_buffer;
  delete sv_buffer;
  delete sw_buffer;
  delete tzc1_buffer;
  delete tzc2_buffer;
  delete tzd1_buffer;
  delete tzd2_buffer;
  delete pw_advection_kernel;
  delete command_queue;
  delete context;
  delete program;
  
  return EXIT_SUCCESS;
}

static long long getTotalFLOPS() {
  long long total_elements_xu=X_SIZE * Y_SIZE * (Z_SIZE-1);
  long long lid_elements=X_SIZE * Y_SIZE;
  long long non_lid_elements=total_elements_xu-lid_elements;
  long long total_elements_w=X_SIZE * Y_SIZE * (Z_SIZE-2);

  long long advectxu_flops=(lid_elements * 17) + (non_lid_elements * 21);
  long long advectw_flops=total_elements_w * 21;
  return (advectxu_flops * 2) + advectw_flops;
}

static float getTimeOfComponent(cl::Event & event) {
  cl_ulong tstart, tstop;

  event.getProfilingInfo(CL_PROFILING_COMMAND_START, &tstart);
  event.getProfilingInfo(CL_PROFILING_COMMAND_END, &tstop);
  return (tstop-tstart)/1.E6;
}

static void initiateData() {
  u_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  v_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  w_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  su_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  sv_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  sw_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z);
  tzc1_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzc2_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzd1_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzd2_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);

  for (unsigned int i=0;i<size_in_x * size_in_y * size_in_z;i++) {
    u_data[i]=23.4;
    v_data[i]=18.8;
    w_data[i]=3.1;
  }

  for (unsigned int i=0;i<size_in_z;i++) {
    tzc1_data[i]=12.3;
    tzc2_data[i]=1.9;
    tzd1_data[i]=5.6;
    tzd2_data[i]=23.8;
  }
  tcx=99.4;
  tcy=12.3;
}

static void execute_entire_kernel_on_device(cl::Event & copyOnEvent, cl::Event & kernelExecutionEvent, cl::Event & copyOffEvent) {
  cl_int err;

  // Queue migration of memory objects from host to device (last argument 0 means from host to device) *buffer_gxyz
  OCL_CHECK(err, err = command_queue->enqueueMigrateMemObjects({*u_buffer, *v_buffer, *w_buffer, 
    *tzc1_buffer, *tzc2_buffer, *tzd1_buffer, *tzd2_buffer}, 0, nullptr, &copyOnEvent));  
  copyOnEvent.wait();

  // Queue kernel execution
  OCL_CHECK(err, err = command_queue->enqueueTask(*pw_advection_kernel, nullptr, &kernelExecutionEvent));  
  // Wait for kernelExecutionEvent to complete
  kernelExecutionEvent.wait();  

  // Queue copy result data back from kernel
  OCL_CHECK(err, err = command_queue->enqueueMigrateMemObjects({*su_buffer, *sv_buffer, *sw_buffer}, CL_MIGRATE_MEM_OBJECT_HOST, nullptr, &copyOffEvent));
  copyOffEvent.wait();  

  // Wait for queue to complete
  OCL_CHECK(err, err = command_queue->finish());
}

static void init_device(char * binary_filename) {
  cl_int err;  

  // Identify the device and create the appropriate context
  std::vector<cl::Device> devices = get_devices("Xilinx");
  devices.resize(1);
  cl::Device device = devices[0];
  OCL_CHECK(err, context=new cl::Context(device, NULL, NULL, NULL, &err));

  // Create the command queue
  OCL_CHECK(err, command_queue=new cl::CommandQueue(*context, device, CL_QUEUE_PROFILING_ENABLE, &err));

  // Read the binary file
  unsigned fileBufSize;
  char* fileBuf = read_binary_file(binary_filename, fileBufSize);  
  cl::Program::Binaries bins{{fileBuf, fileBufSize}};

  // Create the program object from the binary and program the FPGA device with it
  OCL_CHECK(err, program=new cl::Program(*context, devices, bins, NULL, &err));

  // Create a handle to the jacobi kernel
  OCL_CHECK(err, pw_advection_kernel=new cl::Kernel(*program, "pw_advection", &err));

  // Allocate global memory buffers
  OCL_CHECK(err, u_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, u_data, &err));
  OCL_CHECK(err, v_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, v_data, &err));
  OCL_CHECK(err, w_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, w_data, &err));
  OCL_CHECK(err, su_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, su_data, &err));
  OCL_CHECK(err, sv_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, sv_data, &err));
  OCL_CHECK(err, sw_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_x * size_in_y * size_in_z, sw_data, &err));
  OCL_CHECK(err, tzc1_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzc1_data, &err));
  OCL_CHECK(err, tzc2_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzc2_data, &err));
  OCL_CHECK(err, tzd1_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzd1_data, &err));
  OCL_CHECK(err, tzd2_buffer=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzd2_data, &err));

  // Set kernel arguments
  OCL_CHECK(err, err = pw_advection_kernel->setArg(0, *u_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(1, *v_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(2, *w_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(3, *su_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(4, *sv_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(5, *sw_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(6, *tzc1_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(7, *tzc2_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(8, *tzd1_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(9, *tzd2_buffer));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(10, tcx));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(11, tcy));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(12, size_in_x));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(13, size_in_y));
  OCL_CHECK(err, err = pw_advection_kernel->setArg(14, size_in_z));
}
