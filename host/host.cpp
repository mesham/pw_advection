#include "host.hpp"
#include <stdio.h>
#include <malloc.h>
#include <sys/time.h>
#include <CL/cl2.hpp>
#include <CL/cl_ext_xilinx.h>

#define X_SIZE 2048
#define Y_SIZE 2048
#define Z_SIZE 64

#define NUM_KERNELS 1

//#define X_SIZE 8
//#define Y_SIZE 73
//#define Z_SIZE 8

typedef double REAL_TYPE;

#define PAGESIZE 4096

int irlength, hazardlength, n_size;

cl::CommandQueue * command_queue;
cl::Context * context;
cl::Program * program;
cl::Kernel * pw_advection_kernel[NUM_KERNELS];
cl::Buffer *u_buffer[NUM_KERNELS], *v_buffer[NUM_KERNELS], *w_buffer[NUM_KERNELS];
cl::Buffer *su_buffer[NUM_KERNELS], *sv_buffer[NUM_KERNELS], *sw_buffer[NUM_KERNELS];
cl::Buffer *tzc1_buffer[NUM_KERNELS], *tzc2_buffer[NUM_KERNELS], *tzd1_buffer[NUM_KERNELS], *tzd2_buffer[NUM_KERNELS];

REAL_TYPE *u_data[NUM_KERNELS], *v_data[NUM_KERNELS], *w_data[NUM_KERNELS], *su_data[NUM_KERNELS], *sv_data[NUM_KERNELS], *sw_data[NUM_KERNELS], *tzc1_data, *tzc2_data, *tzd1_data, *tzd2_data;
REAL_TYPE tcx, tcy;

int size_in_x=X_SIZE+4, size_in_y=Y_SIZE+4, size_in_z=Z_SIZE;

static void init_device(char*);
static void execute_entire_kernel_on_device(cl::Event*, cl::Event*, cl::Event*);
static float getMaxTimeOfComponent(cl::Event*);
static float getTimeOfComponent(cl::Event&);
static void initiateData();
static long long getTotalFLOPS();
static char * getKernelName(const char*, int, char*);

int main(int argc, char * argv[]) {      
  cl::Event copyOnEvent[NUM_KERNELS], kernelExecutionEvent[NUM_KERNELS], copyOffEvent[NUM_KERNELS];

  printf("Advecting with compute domain X=%d Y=%d Z=%d, total domain size of X=%d Y=%d Z=%d\n", X_SIZE, Y_SIZE, Z_SIZE, size_in_x, size_in_y, size_in_z);

  initiateData();
  init_device(argv[1]);

  execute_entire_kernel_on_device(copyOnEvent, kernelExecutionEvent, copyOffEvent);
#ifdef DISPLAYRESULTS  
  display_results();
#endif

  float copyOnTime=getMaxTimeOfComponent(copyOnEvent);  
  float kernelTime=getMaxTimeOfComponent(kernelExecutionEvent);  
  float copyOffTime=getMaxTimeOfComponent(copyOffEvent);
  
  printf("Execution complete, total runtime : %.3f ms, (%.3f ms xfer on, %.3f ms execute, %.3f ms xfer off)\n", copyOnTime+kernelTime+copyOffTime, copyOnTime, kernelTime, copyOffTime);  

  double totalFLOPS=(getTotalFLOPS() / ((copyOnTime+kernelTime+copyOffTime) /1000)) / 1024 / 1024 / 1024;
  double kernelFLOPS=(getTotalFLOPS() / (kernelTime / 1000)) / 1024 / 1024 / 1024;
  printf("Overall GFLOPS %.2f, kernel GFLOPS %.2f\n", totalFLOPS, kernelFLOPS);

  for (int i=0;i<NUM_KERNELS;i++) {
    delete u_buffer[i];
    delete v_buffer[i];
    delete w_buffer[i];
    delete su_buffer[i];
    delete sv_buffer[i];
    delete sw_buffer[i];
    delete tzc1_buffer[i];
    delete tzc2_buffer[i];
    delete tzd1_buffer[i];
    delete tzd2_buffer[i];
    delete pw_advection_kernel[i];
  }
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

static float getMaxTimeOfComponent(cl::Event * event) {
  float max_time=0.0, new_time;
  for (int i=0;i<NUM_KERNELS;i++) {
    new_time=getTimeOfComponent(event[i]);
    if (new_time > max_time) max_time=new_time;
  }
  return max_time;
}

static float getTimeOfComponent(cl::Event & event) {
  cl_ulong tstart, tstop;

  event.getProfilingInfo(CL_PROFILING_COMMAND_START, &tstart);
  event.getProfilingInfo(CL_PROFILING_COMMAND_END, &tstop);
  return (tstop-tstart)/1.E6;
}

static void initiateData() {
  unsigned int partial_kernel_x_size=size_in_x/NUM_KERNELS;
  size_t cube_size=partial_kernel_x_size * size_in_y * size_in_z;

  for (int i=0;i<NUM_KERNELS;i++) {
    u_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);
    v_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);
    w_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);
    su_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);
    sv_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);
    sw_data[i]=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * cube_size);  

    for (unsigned int j=0;j<cube_size;j++) {
      (u_data[i])[j]=23.4;
      (v_data[i])[j]=18.8;
      (w_data[i])[j]=3.1;
    }
  }

  tzc1_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzc2_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzd1_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);
  tzd2_data=(REAL_TYPE*) memalign(PAGESIZE, sizeof(REAL_TYPE) * size_in_z);  

  for (unsigned int i=0;i<size_in_z;i++) {
    tzc1_data[i]=12.3;
    tzc2_data[i]=1.9;
    tzd1_data[i]=5.6;
    tzd2_data[i]=23.8;
  }
  tcx=99.4;
  tcy=12.3;
}

static void execute_entire_kernel_on_device(cl::Event * copyOnEvents, cl::Event * kernelExecutionEvents, cl::Event * copyOffEvents) {
  cl_int err;

  // Queue migration of memory objects from host to device (last argument 0 means from host to device) *buffer_gxyz
  for (int i=0;i<NUM_KERNELS;i++) {
    OCL_CHECK(err, err = command_queue->enqueueMigrateMemObjects({*u_buffer[i], *v_buffer[i], *w_buffer[i], 
      *tzc1_buffer[i], *tzc2_buffer[i], *tzd1_buffer[i], *tzd2_buffer[i]}, 0, nullptr, &copyOnEvents[i]));
  }  

  // Queue kernel execution
  for (int i=0;i<NUM_KERNELS;i++) {
    std::vector<cl::Event> kernel_wait_events;
    kernel_wait_events.push_back(copyOnEvents[i]);
    OCL_CHECK(err, err = command_queue->enqueueTask(*pw_advection_kernel[i], &kernel_wait_events, &kernelExecutionEvents[i]));
  }  

  // Queue copy result data back from kernel
  for (int i=0;i<NUM_KERNELS;i++) {
    std::vector<cl::Event> kernel_wait_events;
    kernel_wait_events.push_back(kernelExecutionEvents[i]);
    OCL_CHECK(err, err = command_queue->enqueueMigrateMemObjects({*su_buffer[i], *sv_buffer[i], *sw_buffer[i]}, CL_MIGRATE_MEM_OBJECT_HOST, &kernel_wait_events, &copyOffEvents[i]));
  }

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
  OCL_CHECK(err, command_queue=new cl::CommandQueue(*context, device, CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE, &err));

  // Read the binary file
  unsigned fileBufSize;
  char name_buff[100];
  char* fileBuf = read_binary_file(binary_filename, fileBufSize);  
  cl::Program::Binaries bins{{fileBuf, fileBufSize}};

  unsigned int partial_kernel_x_size=size_in_x/NUM_KERNELS;

  // Create the program object from the binary and program the FPGA device with it
  OCL_CHECK(err, program=new cl::Program(*context, devices, bins, NULL, &err));

  for (int i=0;i<NUM_KERNELS;i++) {
    // Create a handle to the jacobi kernel    
    OCL_CHECK(err, pw_advection_kernel[i]=new cl::Kernel(*program, getKernelName("pw_advection", i, name_buff), &err));

    size_t cube_size=partial_kernel_x_size * size_in_y * size_in_z;

    // Allocate global memory buffers
    OCL_CHECK(err, u_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, u_data[i], &err));
    OCL_CHECK(err, v_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, v_data[i], &err));
    OCL_CHECK(err, w_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, w_data[i], &err));
    OCL_CHECK(err, su_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, su_data[i], &err));
    OCL_CHECK(err, sv_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, sv_data[i], &err));
    OCL_CHECK(err, sw_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * cube_size, sw_data[i], &err));
    OCL_CHECK(err, tzc1_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzc1_data, &err));
    OCL_CHECK(err, tzc2_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzc2_data, &err));
    OCL_CHECK(err, tzd1_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzd1_data, &err));
    OCL_CHECK(err, tzd2_buffer[i]=new cl::Buffer(*context, CL_MEM_USE_HOST_PTR, sizeof(REAL_TYPE) * size_in_z, tzd2_data, &err));

    // Set kernel arguments
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(0, *u_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(1, *v_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(2, *w_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(3, *su_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(4, *sv_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(5, *sw_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(6, *tzc1_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(7, *tzc2_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(8, *tzd1_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(9, *tzd2_buffer[i]));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(10, tcx));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(11, tcy));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(12, partial_kernel_x_size));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(13, size_in_y));
    OCL_CHECK(err, err = pw_advection_kernel[i]->setArg(14, size_in_z));
  }
}

static char * getKernelName(const char * base, int index, char * buffer) {
  sprintf(buffer, "%s:{%s_%d}", base, base, index+1);
  return buffer;
}
