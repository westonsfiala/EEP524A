#pragma once
#include <string>

static const std::string output_global_id = "\
__kernel void output_global_id(\
__global float* out\
)\
{ \
    int x_id = get_global_id(0);\
\
    out[x_id] = x_id;\
}";

static const std::string output_global_id_kernel_name = "output_global_id";