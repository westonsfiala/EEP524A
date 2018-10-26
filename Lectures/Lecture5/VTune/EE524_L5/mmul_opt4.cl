// Matrix Multiplication case study: Optimization #4 (OPT4)
// modification of OPT #3
// move entire row of A into private memory (registers) to reduce global mem accesses
// move entire col of B into local memory shared by WG

// V1: local memory passed from Host as kernel argument local pointer

//__kernel void mmul_opt4(const int N, const int P, __global float* A, __global float* B, __global float* C, __local float* Bloc)
__kernel void mmul_opt4(const int N, const int P, __global float* A, __global float* B, __global float* C)
{
	int i,j,k;
	i = get_global_id(0);
	int iloc = get_local_id(0);
	int nloc = get_local_size(0);

	float tmp;
	float Apriv[512];
	__local float Bloc[512];

	// copy global row to WI private registers
	for(k=0; k<N; k++)
		Apriv[k] = A[i*N+k];

	// each WI computes entire row i of output matrix C
	for(j=0; j<N; j++)
	{ 
		// copy global col to WG shared local memory (SLM)
		// WIs copy column in parallel, striding by WG size
		for(k=iloc; k<N; k+=nloc)
			Bloc[k]= B[k*N+j];

		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync

		tmp = 0.0f;
		for(k=0; k < N; k++) {
			tmp += Apriv[k] * Bloc[k];
		}
		C[i*N+j] = tmp;
		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync
	}
}


// V2: local memory declared inside kernel function space
/*
__kernel void mmul_opt4(const int N, __global float* A, __global float* B, __global float* C)
{
	local float	Bloc[512];
	int i,j,k;
	i = get_global_id(0);
	int iloc = get_local_id(0);
	int nloc = get_local_size(0);

	float tmp;
	float Apriv[1024];

	// copy global row to WI private registers
	for(k=0; k<N; k++)
		Apriv[k] = A[i*N+k];

	// each WI computes entire row i of output matrix C
	for(j=0; j<N; j++)
	{ 
		// copy global col to WG shared local memory (SLM)
		// WIs copy column in parallel, striding by WG size
		for(k=iloc; k<N; k+=nloc)
			Bloc[k]= B[k*N+j];

		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync

		tmp = 0.0f;
		for(k=0; k < N; k++) {
			tmp += Apriv[k] * Bloc[k];
		}
		C[i*N+j] = tmp;
		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync
	}
}
*/