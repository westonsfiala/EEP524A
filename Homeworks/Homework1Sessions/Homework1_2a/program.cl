__kernel void saxpy1d(
__global const float* scalar,
__global const float* in_a,
__global const float* in_b,
__global float* out
)
{ 
    int x_id = get_global_id(0);

    out[x_id] = in_a[x_id] * *scalar + in_b[x_id];
}