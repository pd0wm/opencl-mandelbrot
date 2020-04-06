#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <iostream>
#include <cassert>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdint>

#include <CL/cl.h>

#include "svgpng.hpp"
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

  cl_context ctx = clCreateContext(NULL, 1, &d, NULL, NULL, &err);
  cl_ok(err);

  cl_command_queue q = clCreateCommandQueue(ctx, d, 0, &err);
  cl_ok(err);

  auto source = read_file("../mandelbrot.cl");
  auto program = build(ctx, d, source);

  const size_t img_width = 1200;
  const size_t img_height = 800;

  // Create kernel
  cl_kernel kernel = clCreateKernel(program, "mandelbrot", &err);
  cl_ok(err);

  cl_image_format format;
  format.image_channel_order = CL_RGBA;
  format.image_channel_data_type = CL_UNSIGNED_INT8;

  cl_image_desc desc;
  desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  desc.image_width = img_width;
  desc.image_height = img_height;
  desc.image_depth = 0;
  desc.image_array_size = 0;
  desc.image_row_pitch = 0;
  desc.image_slice_pitch = 0;
  desc.num_mip_levels = 0;
  desc.num_samples = 0;
  desc.buffer = NULL;

  auto cl_img = clCreateImage(ctx, CL_MEM_WRITE_ONLY, &format, &desc, NULL, &err);
  cl_ok(err);

  cl(clSetKernelArg(kernel, 0, sizeof(cl_img), &cl_img));

  // Queue kernel
  size_t global[2] = {img_width, img_height};
  size_t local[2] = {16, 16};
  cl(clEnqueueNDRangeKernel(q, kernel, 2, NULL, global, local, 0, NULL, NULL));

  // Read output
  uint8_t * img = new uint8_t[img_width * img_height * 4];

  size_t origin[3] = {0, 0, 0};
  size_t depth[3] = {img_width, img_height, 1};
  cl(clEnqueueReadImage(q, cl_img, CL_TRUE,
                        origin, depth, 0, 0, img,
                        0, NULL, NULL));

  // Wait for opencl to finish
  cl(clFlush(q));
  cl(clFinish(q));

  // Write image
  FILE* fp = fopen("mandelbrot.png", "wb");
  svpng(fp, img_width, img_height, img, 1);
  fclose(fp);

  // Cleanup
  delete[] img;
  cl(clReleaseKernel(kernel));
  cl(clReleaseProgram(program));
  cl(clReleaseMemObject(cl_img));
  cl(clReleaseCommandQueue(q));
  cl(clReleaseContext(ctx));
}
