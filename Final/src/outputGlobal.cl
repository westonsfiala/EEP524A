
#include "../../src/clcomplex.h"

kernel void OutputGlobal (
    global const float2* input,
    global float2* output
)
{
    int xId = get_global_id(0);

    float2 order = { xId, 0.0f };

    output[xId] = cpowf(input[xId], order) + order;
}