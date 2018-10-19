__kernel void vec3crossproduct(
__global const int4* in_a,
__global const int4* in_b,
__global int4* out
)
{
    int x_id = get_global_id(0);

    int4 a_in = in_a[x_id];

    int4 b_in = in_b[x_id];

    out[x_id].x = a_in.y * b_in.z - a_in.z * b_in.y;
    out[x_id].y = a_in.z * b_in.x - a_in.x * b_in.z;
    out[x_id].z = a_in.x * b_in.y - a_in.y * b_in.x;
    out[x_id].w = 0;
    
    // When using floats use the built in cross product function.
    //float4 cross_product = cross(a_in, b_in);
}