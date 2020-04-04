#pragma once
#include <CL/cl.h>

#define cl(...)    cl_assert((__VA_ARGS__), __FILE__, __LINE__, true);
#define cl_ok(err) cl_assert(err, __FILE__, __LINE__, true);

char const * clGetErrorString(cl_int const err);
cl_int cl_assert(cl_int const code, char const * const file, int const line, bool const abort);

