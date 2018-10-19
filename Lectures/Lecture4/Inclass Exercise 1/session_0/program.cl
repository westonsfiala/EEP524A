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

