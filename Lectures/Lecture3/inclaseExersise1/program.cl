__kernel void mmul_v1( 
const int N,
__global float *A,
__global float *B, 
__global float *C)
{
    int i, j, k;
    i = get_global_id(0);
    j = get_global_id(1);

    for (k = 0; k < N; k++) {
        C[i*N+j] += A[i*N+k] * B[k*N+j];
    }
}

__kernel void mmul_v2( 
const int N,
__global float *A,
__global float *B, 
__global float *C)
{
    int i, j, k;
    i = get_global_id(0);
    j = get_global_id(1);

    float accum = 0.0f;

    for (k = 0; k < N; k++) {
        accum += A[i*N+k] * B[k*N+j];
    }

    C[i*N+j] += accum;
}