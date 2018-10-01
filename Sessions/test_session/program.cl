__kernel void vecadd1d (
__global const float *a,
__global const float *b,
__global float *c
)
{ 
    int id = get_global_id(0);
    c[id] = a[id] + b[id];
}
