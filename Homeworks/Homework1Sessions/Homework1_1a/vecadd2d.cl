__kernel void vecadd2d(
__global const float* in_a,
__global const float* in_b,
__global float* out
)
{ 
    int x_id = get_global_id(0);
    int y_id = get_global_id(1);

    int y_size = get_global_size(1);

    out[x_id*y_size + y_id] = in_a[x_id*y_size + y_id] + in_b[x_id*y_size + y_id];
}