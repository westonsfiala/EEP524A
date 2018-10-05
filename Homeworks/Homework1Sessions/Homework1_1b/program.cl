__kernel void vecadd3d(
__global const float* in_a,
__global const float* in_b,
__global float* out
)
{ 
    int x_id = get_global_id(0);
    int y_id = get_global_id(1);
    int z_id = get_global_id(2);

    int x_size = get_global_size(0);
    int y_size = get_global_size(1);

    out[z_id*y_size*x_size + y_id*x_size + x_id] = in_a[z_id*y_size*x_size + y_id*x_size + x_id] + in_b[z_id*y_size*x_size + y_id*x_size + x_id];
}