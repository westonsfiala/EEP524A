__kernel void vecadd_anyD(global float *a, global float *b, global float *c)
{ 
	uint x,y,z,nx,ny,idx;
	uint dims = get_work_dim();

	if(dims == 1) // Interpret 1D buffer data as 1D array
	{
		x = get_global_id(0);
		nx = get_global_size(0);
		idx = x;
	}
	else if(dims == 2) // Interpret 1D buffer data as 2D flattened grid, in row-major order
	{
		x = get_global_id(0);
		y = get_global_id(1);
		nx = get_global_size(0);
		idx = nx*y + x;
	}
	else if(dims == 3) // Interpret 1D buffer data as 3D flattened grid, in row-major order with increasing z-tiles
	{
		x = get_global_id(0);
		y = get_global_id(1);
		z = get_global_id(2);
		nx = get_global_size(0);
		ny = get_global_size(1);
		idx = nx*ny*z + nx*y + x;
	}
	else
	{
		// invalid dimensionality of NDRange
		return;
	}

	c[idx] = a[idx]+b[idx];
	//c[16] = (float)dims;  // HACK write dimensions to output
}
