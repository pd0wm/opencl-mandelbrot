#include <iostream>
#include <cassert>
#include <string>

#include <CL/cl.h>

#include "assert_cl.hpp"

#define UNUSED(x) (void)(x)

std::string get_device_info(cl_device_id device, cl_device_info param_name){
  size_t value_sz;
  cl(clGetDeviceInfo(device, param_name, 0, NULL, &value_sz));
  char * value = new char[value_sz];
  cl(clGetDeviceInfo(device, param_name, value_sz, value, NULL));

  std::string r = std::string(value);

  delete[] value;

  return r;
}

cl_device_id get_first_device(void){
  cl_device_id result = NULL;


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
    cl(clGetDeviceIDs(platforms[platform_idx], CL_DEVICE_TYPE_ALL, 1, &result, NULL));
    delete[] platforms;
    return result;
  }

  delete[] platforms;

  return result;
}


int main( int argc, char *argv[] ) {
  UNUSED(argc);
  UNUSED(argv);

  cl_device_id d = get_first_device();
  assert(d);

  std::cout << "Device name: " << get_device_info(d, CL_DEVICE_NAME) << std::endl;
  std::cout << "OpenCL C version: " << get_device_info(d, CL_DEVICE_OPENCL_C_VERSION) << std::endl;

  cl_uint compute_units;
  cl(clGetDeviceInfo(d, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL));
  std::cout << "Compute units: " << compute_units << std::endl;

}
