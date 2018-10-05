__kernel void vecadd2d(
__global const float* in_a,
__global const float* in_b,
__global float* out
)
{ 
    int x_id = get_global_id(0);
    int y_id = get_global_id(1);

    int x_size = get_global_size(0);

    out[y_id*x_size + x_id] = in_a[y_id*x_size + x_id] + in_b[y_id*x_size + x_id];
}