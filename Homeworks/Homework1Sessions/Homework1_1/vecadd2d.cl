__kernel void vecadd2d(
__global const float4* in_a,
__global const float4* in_b,
__global float4* out
)
{ 
    int x_id = get_global_id(0);

    out[x_id] = in_a[x_id] + in_b[x_id];
}