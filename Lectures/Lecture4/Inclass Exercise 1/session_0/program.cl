__kernel void mmul_opt2(
const int N,
__global float *A,
__global float *B,
__global float *C)
{ 
    int j, k;
    int i = get_global_id(0);
    float tmp;

    for (j = 0; j < N; j++) {
        tmp = 0.0f;

        for (k = 0; k < N; k++)
        {
            tmp += A[i*N+k]*B[k*N+j];
        }

        C[i*N+j] = tmp;
    }
}

__kernel void mmul_opt3(
const int N,
__global float *A,
__global float *B,
__global float *C)
{
    int j, k;
    int i =
    get_global_id(0);
    float tmp;
    float Awrk[512];

    for (k = 0; k < N; k++)
        Awrk[k] = A[i*N+k];

    for (j = 0; j < N; j++) {
        tmp = 0.0f;

        for (k = 0; k < N; k++)
            tmp += Awrk[k]*B[k*N+j];

        C[i*N+j] += tmp;
    }
}

__kernel void mmul_opt4(
const int N,
__global float *A,
__global float *B,
__global float *C,
__local float *Bwrk
)
{
    int j, k;
    int i =
    get_global_id(0);
    int iloc = get_local_id(0);
    int nloc = get_local_size(0);
    float tmp;
    float Awrk[512];

    for (k = 0; k < N; k++)
        Awrk[k] = A[i*N+k];

    for (j = 0; j < N; j++) {

        for (k = iloc; k<N; k+=nloc)
        { 
            Bwrk[k] = B[k*N + j];
        }

        barrier(CLK_LOCAL_MEM_FENCE);

        tmp = 0.0f;

        for (k = 0; k < N; k++)
            tmp += Awrk[k]*Bwrk[k];

        C[i*N+j] += tmp;

        barrier(CLK_LOCAL_MEM_FENCE);
    }
}

