__kernel void output_global_id(
__global float* out
)
{ 
    int x_id = get_global_id(0);

    out[x_id] = x_id;
}