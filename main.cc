#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>

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


  auto source = read_file("../test.cl");
  std::cout << source << std::endl;

  auto program = build(ctx, d, source);

  cl(clReleaseContext(ctx));
}
