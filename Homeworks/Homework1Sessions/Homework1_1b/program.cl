__kernel void vecadd3d(
__global const float* in_a,
__global const float* in_b,
__global float* out
)
{ 
    int x_id = get_global_id(0);
    int y_id = get_global_id(1);
    int z_id = get_global_id(2);

    int y_size = get_global_size(1);
    int z_size = get_global_size(2);

    out[x_id*y_size*z_size + y_id*z_size + z_id] = in_a[x_id*y_size*z_size + y_id*z_size + z_id] + in_b[x_id*y_size*z_size + y_id*z_size + z_id];
}