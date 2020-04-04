#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>

#include <CL/cl.h>

#include "assert_cl.hpp"

#define UNUSED(x) (void)(x)

std::string get_device_info_string(cl_device_id device, cl_device_info param_name){
  size_t value_sz;
  cl(clGetDeviceInfo(device, param_name, 0, NULL, &value_sz));
  char * value = new char[value_sz];
  cl(clGetDeviceInfo(device, param_name, value_sz, value, NULL));

  std::string r = std::string(value);
  delete[] value;
  return r;
}

cl_device_id get_first_device(void){
  cl_uint num_platforms;
  cl(clGetPlatformIDs(0, NULL, &num_platforms));

  // Get platforms
  cl_platform_id * platforms = new cl_platform_id[num_platforms];
  cl(clGetPlatformIDs(num_platforms, platforms, NULL));

  // Get devices
  for (size_t platform_idx = 0; platform_idx < num_platforms; platform_idx++){
    // Get number of devices
    cl_uint num_devices;
    cl(clGetDeviceIDs(platforms[platform_idx], CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices));

    if (num_devices == 0){
      continue;
    }

    // Get first device
    cl_device_id result = NULL;
    cl(clGetDeviceIDs(platforms[platform_idx], CL_DEVICE_TYPE_ALL, 1, &result, NULL));
    delete[] platforms;
    return result;
  }

  delete[] platforms;
  return NULL;
}

std::string read_file(const char * fn){
  std::ifstream f(fn);
  std::stringstream buf;
  buf << f.rdbuf();
  return buf.str();
}

cl_program build(const cl_context ctx, const cl_device_id device, std::string source){
  cl_int err;
  const char * string = source.c_str();
  const size_t length = source.size();
  cl_program program = clCreateProgramWithSource(ctx, 1, &string, &length, &err);
  cl_ok(err);

  const char * options = "";
  err = clBuildProgram(program, 1, &device, options, NULL, NULL);

  if (err == CL_BUILD_PROGRAM_FAILURE){
    std::cout << "Build failure:" << std::endl;

    size_t value_sz;
    cl(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &value_sz));

    char * value = new char[value_sz];
    cl(clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, value_sz, value, NULL));
    std::cout << value;

    delete[] value;
    assert(false);
  }

  return program;
}


int main( int argc, char *argv[] ) {
  UNUSED(argc);
  UNUSED(argv);

  cl_int err;

  cl_device_id d = get_first_device();
  assert(d);

  std::cout << "Device name: " << get_device_info_string(d, CL_DEVICE_NAME) << std::endl;
  std::cout << "OpenCL C version: " << get_device_info_string(d, CL_DEVICE_OPENCL_C_VERSION) << std::endl;

  cl_uint compute_units;
  cl(clGetDeviceInfo(d, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL));
  std::cout << "Compute units: " << compute_units << std::endl;

  cl_context ctx = clCreateContext(NULL, 1, &d, NULL, NULL, &err);
  cl_ok(err);

  cl_command_queue q = clCreateCommandQueue(ctx, d, 0, &err);
  cl_ok(err);

  auto source = read_file("../test.cl");
  std::cout << source << std::endl;

  auto program = build(ctx, d, source);

  // Build input buffers
  const size_t sz = 128;
  const size_t buf_sz = sizeof(cl_int) * sz;
  cl_int * A = new int[sz];
  cl_int * B = new int[sz];
  cl_int * out = new int[sz];

  for (size_t i = 0; i < sz; i++){
    A[i] = i;
    B[i] = 100;
  }
  auto A_cl = clCreateBuffer(ctx, CL_MEM_READ_ONLY, buf_sz, NULL, &err);
  cl_ok(err);
  auto B_cl = clCreateBuffer(ctx, CL_MEM_READ_ONLY, buf_sz, NULL, &err);
  cl_ok(err);
  auto out_cl = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, buf_sz, NULL, &err);
  cl_ok(err);

  // Create kernel
  cl_kernel kernel = clCreateKernel(program, "vector_add", &err);
  cl_ok(err);

  // Set kernel arguments
  cl(clSetKernelArg(kernel, 0, sizeof(A_cl), &A_cl));
  cl(clSetKernelArg(kernel, 1, sizeof(B_cl), &B_cl));
  cl(clSetKernelArg(kernel, 2, sizeof(out_cl), &out_cl));

  // Copy A and B into device memory
  cl(clEnqueueWriteBuffer(q, A_cl, CL_TRUE, 0, buf_sz, A, 0, NULL, NULL));
  cl(clEnqueueWriteBuffer(q, B_cl, CL_TRUE, 0, buf_sz, B, 0, NULL, NULL));

  // Queue kernel
  size_t N = 8;
  cl(clEnqueueNDRangeKernel(q, kernel, 1, NULL, &sz, &N, 0, NULL, NULL));

  // Read output
  cl(clEnqueueReadBuffer(q, out_cl, CL_TRUE, 0, buf_sz, out, 0, NULL, NULL));


  // Wait for opencl to finish
  cl(clFlush(q));
  cl(clFinish(q));

  for (size_t i = sz - 10; i < sz; i++){
    std::cout << "out[" << i << "]: " << out[i] << std::endl;
  }

  // Cleanup
  delete[] A;
  delete[] B;
  delete[] out;


  cl(clReleaseKernel(kernel));
  cl(clReleaseProgram(program));
  cl(clReleaseMemObject(A_cl));
  cl(clReleaseMemObject(B_cl));
  cl(clReleaseMemObject(out_cl));
  cl(clReleaseCommandQueue(q));
  cl(clReleaseContext(ctx));
}
