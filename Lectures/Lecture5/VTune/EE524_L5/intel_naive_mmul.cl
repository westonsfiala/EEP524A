__kernel void intel_mmul_opt1(const int N, const int P, __global const float * restrict A,
                         __global const float * restrict B,
                         __global float * restrict C)
{
	int row = get_global_id(1);
	int col = get_global_id(0);
	float sum = 0.0f;
	for (int i = 0; i < N; i++) 
		sum += A[row*N+i] * B[i*N+col];

	C[row*N+col] = sum;
}

__kernel void intel_mmul_naive(const int N, const int P, __global const float * A,
                         __global const float * B,
                         __global float * C)
{
	int row = get_global_id(1);
	int col = get_global_id(0);
	C[row*N+col] = 0.0f;
	for (int i = 0; i < N; i++) 
		C[row*N+col] += A[row*N+i] * B[i*N+col];

}