__kernel void vec3dotproduct(
__global const int4* in_a,
__global const int4* in_b,
__global int* out
)
{
    int x_id = get_global_id(0);

    int4 a_in = in_a[x_id];

    int4 b_in = in_b[x_id];

    out[x_id] = a_in.x * b_in.x + a_in.y * b_in.y + a_in.z * b_in.z;
    
    // If using floats use the built in dot method.
    //float4 dot_product = dot(a_in,b_in);
}