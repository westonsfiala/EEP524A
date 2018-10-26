// Matrix Multiplication case study: Optimization #5 (OPT5) blocked/tiled/partitioned matrices
//
// All matrixes must be NxN
// N and P must be powers of 2, P must be a square (4,16,64)
// partition INPUT (A,B) and OUTPUT (C) matrices into PxP sub-matrices, each N/P x N/P
//
// there will be P^2 workgroups
// each workgroup will have N/P workitems
// move entire row of A into private memory (registers) to reduce global mem accesses
// move entire col of B into local memory shared by WG

// Version 0: (V0) - Initial Implementation
__attribute__((reqd_work_group_size(4,4,1)))
__kernel void mmul_opt5_tiled_v0(const int N, const int P, __global float* restrict A, __global float* restrict B, __global float* restrict C)
{
	int i,k;
	int iloc = get_local_linear_id();
	int wgRow = get_group_id(0);		// using dim-0 as row dim
	int wgCol = get_group_id(1);		// using dim-1 as column dim
	//int glid = get_global_linear_id();

	int blkSize = N/P;
	int blkRowOffset = blkSize*wgRow;
	int blkColOffset = blkSize*wgCol;

	//printf("<mmul_opt5_tiled> glid = %d: iloc: %d, wgRow: %d, wgCol: %d, blocksize: %d, blkoffset: (row: %d, col: %d)\n",glid,iloc,wgRow,wgCol,blkSize,blkRowOffset,blkColOffset);

	float outtmp[16] = {0.0f};	// this value needs to be updated based on N and P
	float Apriv[16] = {0.0f};	// this value needs to be updated based on N and P
	local float	Bloc[256];

	// each WI computes a row i of partition sub-matrix C(wgRow,wgCol)
	for(int nP=0; nP<P; nP++) // for each partition in INPUT matrices row,col
	{ 
		// copy global row of partition submatrix A to WI private registers
		for(k=0; k<blkSize; k++)
			Apriv[k] = A[(blkRowOffset+iloc)*N + nP*blkSize + k];
 
		// copy global col of partition submatrix B to WG shared local memory (SLM)
		// WIs copy all columns of partition submatrix in parallel
		for(k=0; k<blkSize; k++)
			Bloc[k*blkSize+iloc]= B[(nP*blkSize+k)*N + blkColOffset + iloc];

		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync

		// each WI compute partition row sub-product
		for(i=0; i < blkSize; i++) {
			for(k=0; k < blkSize; k++) {
				outtmp[i] += Apriv[k] * Bloc[k*blkSize+i];
			}
		}
		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync
	}
	// write final results back to global 
	for(i=0; i< blkSize; i++)
		C[(blkRowOffset+iloc)*N + blkColOffset + i] = outtmp[i];
}

// Version 1 (V1) ====================================
// REQUIRES BLOCKSIZE = 16 (16x16 blocks)
// Unroll main submatrix row computation inner loop, and use mad()

__attribute__((reqd_work_group_size(4,4,1)))
__kernel void mmul_opt5_tiled_v1(const int N, const int P, __global float* A, __global float* B, __global float* C)
{
	int i,k;
	int iloc = get_local_linear_id();
	int wgRow = get_group_id(0);		// using dim-0 as row dim
	int wgCol = get_group_id(1);		// using dim-1 as column dim
	//int glid = get_global_linear_id();

	int blkSize = N/P;
	int blkRowOffset = blkSize*wgRow;
	int blkColOffset = blkSize*wgCol;

	//printf("<mmul_opt5_tiled> glid = %d: iloc: %d, wgRow: %d, wgCol: %d, blocksize: %d, blkoffset: (row: %d, col: %d)\n",glid,iloc,wgRow,wgCol,blkSize,blkRowOffset,blkColOffset);

	float outtmp[16] = {0.0f};	// this value needs to be updated based on N and P
	float Apriv[16] = {0.0f};	// this value needs to be updated based on N and P
	local float	Bloc[256];

	// each WI computes a row i of partition sub-matrix C(wgRow,wgCol)
	for(int nP=0; nP<P; nP++) // for each partition in INPUT matrices row,col
	{ 
		// copy global row of partition submatrix A to WI private registers
		for(k=0; k<blkSize; k++)
			Apriv[k] = A[(blkRowOffset+iloc)*N + nP*blkSize + k];
 
		// copy global col of partition submatrix B to WG shared local memory (SLM)
		// WIs copy all columns of partition submatrix in parallel
		for(k=0; k<blkSize; k++)
			Bloc[k*blkSize+iloc]= B[(nP*blkSize+k)*N + blkColOffset + iloc];

		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync

		// each WI compute partition row sub-product
		for(i=0; i < blkSize; i++) {
			outtmp[i] = mad(Apriv[0],Bloc[0*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[1],Bloc[1*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[2],Bloc[2*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[3],Bloc[3*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[4],Bloc[4*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[5],Bloc[5*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[6],Bloc[6*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[7],Bloc[7*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[8],Bloc[8*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[9],Bloc[9*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[10],Bloc[10*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[11],Bloc[11*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[12],Bloc[12*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[13],Bloc[13*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[14],Bloc[14*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[15],Bloc[15*blkSize+i],outtmp[i]);
		}
		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync
	}
	// write final results back to global 
	for(i=0; i< blkSize; i++)
		C[(blkRowOffset+iloc)*N + blkColOffset + i] = outtmp[i];

}

// Version 2 (V2) ====================================
// REQUIRES BLOCKSIZE = 16 (16x16 blocks)
// V1 + unroll more loops
// Unroll main submatrix row computation inner loop, and use mad()

__attribute__((reqd_work_group_size(4,4,1)))
//__kernel void mmul_opt5_tiled_v2(const int N, const int P, __global float* A, __global float* B, __global float* C)
__kernel void mmul_opt5_tiled_v2(const int N, const int P, __global float* restrict A, __global float* restrict B, __global float* restrict C)
{
	int i,k;
	int iloc = get_local_linear_id();
	int wgRow = get_group_id(0);		// using dim-0 as row dim
	int wgCol = get_group_id(1);		// using dim-1 as column dim
	//int glid = get_global_linear_id();

	int blkSize = N/P;
	int blkRowOffset = blkSize*wgRow;
	int blkColOffset = blkSize*wgCol;

	//printf("<mmul_opt5_tiled> glid = %d: iloc: %d, wgRow: %d, wgCol: %d, blocksize: %d, blkoffset: (row: %d, col: %d)\n",glid,iloc,wgRow,wgCol,blkSize,blkRowOffset,blkColOffset);

	float outtmp[16] = {0.0f};	// this value needs to be updated based on N and P
	float Apriv[16] = {0.0f};	// this value needs to be updated based on N and P
	local float	Bloc[256];

	// each WI computes a row i of partition sub-matrix C(wgRow,wgCol)
	for(int nP=0; nP<P; nP++) // for each partition in INPUT matrices row,col
	{ 
		// copy global row of partition submatrix A to WI private registers
		Apriv[0] = A[(blkRowOffset+iloc)*N + nP*blkSize + 0];
		Apriv[1] = A[(blkRowOffset+iloc)*N + nP*blkSize + 1];
		Apriv[2] = A[(blkRowOffset+iloc)*N + nP*blkSize + 2];
		Apriv[3] = A[(blkRowOffset+iloc)*N + nP*blkSize + 3];
		Apriv[4] = A[(blkRowOffset+iloc)*N + nP*blkSize + 4];
		Apriv[5] = A[(blkRowOffset+iloc)*N + nP*blkSize + 5];
		Apriv[6] = A[(blkRowOffset+iloc)*N + nP*blkSize + 6];
		Apriv[7] = A[(blkRowOffset+iloc)*N + nP*blkSize + 7];
		Apriv[8] = A[(blkRowOffset+iloc)*N + nP*blkSize + 8];
		Apriv[9] = A[(blkRowOffset+iloc)*N + nP*blkSize + 9];
		Apriv[10] = A[(blkRowOffset+iloc)*N + nP*blkSize + 10];
		Apriv[11] = A[(blkRowOffset+iloc)*N + nP*blkSize + 11];
		Apriv[12] = A[(blkRowOffset+iloc)*N + nP*blkSize + 12];
		Apriv[13] = A[(blkRowOffset+iloc)*N + nP*blkSize + 13];
		Apriv[14] = A[(blkRowOffset+iloc)*N + nP*blkSize + 14];
		Apriv[15] = A[(blkRowOffset+iloc)*N + nP*blkSize + 15];

		// copy global col of partition submatrix B to WG shared local memory (SLM)
		// WIs copy all columns of partition submatrix in parallel
		Bloc[0*blkSize+iloc]= B[(nP*blkSize+0)*N + blkColOffset + iloc];
		Bloc[1*blkSize+iloc]= B[(nP*blkSize+1)*N + blkColOffset + iloc];
		Bloc[2*blkSize+iloc]= B[(nP*blkSize+2)*N + blkColOffset + iloc];
		Bloc[3*blkSize+iloc]= B[(nP*blkSize+3)*N + blkColOffset + iloc];
		Bloc[4*blkSize+iloc]= B[(nP*blkSize+4)*N + blkColOffset + iloc];
		Bloc[5*blkSize+iloc]= B[(nP*blkSize+5)*N + blkColOffset + iloc];
		Bloc[6*blkSize+iloc]= B[(nP*blkSize+6)*N + blkColOffset + iloc];
		Bloc[7*blkSize+iloc]= B[(nP*blkSize+7)*N + blkColOffset + iloc];
		Bloc[8*blkSize+iloc]= B[(nP*blkSize+8)*N + blkColOffset + iloc];
		Bloc[9*blkSize+iloc]= B[(nP*blkSize+9)*N + blkColOffset + iloc];
		Bloc[10*blkSize+iloc]= B[(nP*blkSize+10)*N + blkColOffset + iloc];
		Bloc[11*blkSize+iloc]= B[(nP*blkSize+11)*N + blkColOffset + iloc];
		Bloc[12*blkSize+iloc]= B[(nP*blkSize+12)*N + blkColOffset + iloc];
		Bloc[13*blkSize+iloc]= B[(nP*blkSize+13)*N + blkColOffset + iloc];
		Bloc[14*blkSize+iloc]= B[(nP*blkSize+14)*N + blkColOffset + iloc];
		Bloc[15*blkSize+iloc]= B[(nP*blkSize+15)*N + blkColOffset + iloc];

		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync

		// each WI compute partition row sub-product
		for(i=0; i < blkSize; i++) {
			outtmp[i] = mad(Apriv[0],Bloc[0*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[1],Bloc[1*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[2],Bloc[2*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[3],Bloc[3*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[4],Bloc[4*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[5],Bloc[5*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[6],Bloc[6*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[7],Bloc[7*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[8],Bloc[8*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[9],Bloc[9*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[10],Bloc[10*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[11],Bloc[11*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[12],Bloc[12*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[13],Bloc[13*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[14],Bloc[14*blkSize+i],outtmp[i]);
			outtmp[i] = mad(Apriv[15],Bloc[15*blkSize+i],outtmp[i]);
		}
		work_group_barrier(CLK_LOCAL_MEM_FENCE); // WG sync
	}
	// write final results back to global 
	C[(blkRowOffset+iloc)*N + blkColOffset + 0] = outtmp[0];
	C[(blkRowOffset+iloc)*N + blkColOffset + 1] = outtmp[1];
	C[(blkRowOffset+iloc)*N + blkColOffset + 2] = outtmp[2];
	C[(blkRowOffset+iloc)*N + blkColOffset + 3] = outtmp[3];
	C[(blkRowOffset+iloc)*N + blkColOffset + 4] = outtmp[4];
	C[(blkRowOffset+iloc)*N + blkColOffset + 5] = outtmp[5];
	C[(blkRowOffset+iloc)*N + blkColOffset + 6] = outtmp[6];
	C[(blkRowOffset+iloc)*N + blkColOffset + 7] = outtmp[7];
	C[(blkRowOffset+iloc)*N + blkColOffset + 8] = outtmp[8];
	C[(blkRowOffset+iloc)*N + blkColOffset + 9] = outtmp[9];
	C[(blkRowOffset+iloc)*N + blkColOffset + 10] = outtmp[10];
	C[(blkRowOffset+iloc)*N + blkColOffset + 11] = outtmp[11];
	C[(blkRowOffset+iloc)*N + blkColOffset + 12] = outtmp[12];
	C[(blkRowOffset+iloc)*N + blkColOffset + 13] = outtmp[13];
	C[(blkRowOffset+iloc)*N + blkColOffset + 14] = outtmp[14];
	C[(blkRowOffset+iloc)*N + blkColOffset + 15] = outtmp[15];
}

// V2: local memory declared inside kernel function space
/*
__kernel void mmul_opt4(const int N, __global float* A, __global float* B, __global float* C)
{
	local float	Bloc[512];
	int i,j,k;
*/