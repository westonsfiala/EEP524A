// Add you host code
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "CL/cl.h"int main(int argc, char** argv){
	cl_uint num_entries;
	cl_platform_id* platforms;
	cl_uint num_platforms;

	// Get the number of platforms
	cl_int success = clGetPlatformIDs(		0, 		nullptr, 		&num_platforms	);	assert(success == CL_SUCCESS);	platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * num_platforms);	// Get the platforms	success = clGetPlatformIDs(		num_entries, 		platforms, 		nullptr	);	assert(success == CL_SUCCESS);	for (int platform_index = 0; platform_index < num_platforms; platform_index++)	{		size_t param_value_size;		// Get the size of the name of the platform.		success = clGetPlatformInfo(
			platforms[platform_index],
			CL_PLATFORM_NAME,
			0,
			nullptr,
			&param_value_size		);		assert(success == CL_SUCCESS);		char* platform_name = (char*)malloc(sizeof(char) * param_value_size);		success = clGetPlatformInfo(
			platforms[platform_index],
			CL_PLATFORM_NAME,
			param_value_size,
			platform_name,
			nullptr		);		assert(success == CL_SUCCESS);		printf(platform_name);		cl_uint num_devices;		// Get the number of devices		success = clGetDeviceIDs(
			platforms[platform_index],
			CL_DEVICE_TYPE_ALL,
			0,
			nullptr,
			&num_devices		);		assert(success == CL_SUCCESS);		cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id) * num_devices);		// Get the number of devices		success = clGetDeviceIDs(
			platforms[platform_index],
			CL_DEVICE_TYPE_ALL,
			num_devices,
			devices,
			&num_devices		);		assert(success == CL_SUCCESS);		for (int device_index = 0; device_index < num_devices; device_index++)		{			// Get info from the devices			size_t param_value_size;			// Get the device name			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_NAME,
				0,
				nullptr,
				&param_value_size			);			assert(success == CL_SUCCESS);			char* device_name = (char*)malloc(param_value_size);			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_NAME,
				param_value_size,
				device_name,
				nullptr			);			assert(success == CL_SUCCESS);			printf(device_name);			// Get the double config			cl_device_fp_config fp_config;			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_DOUBLE_FP_CONFIG,
				sizeof(cl_device_fp_config),
				&fp_config,
				nullptr			);			assert(success == CL_SUCCESS);			printf("Double FP Config : %X", fp_config);			// Get the preferred vector width float			cl_uint prefered_vector_width_float;			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
				sizeof(cl_uint),
				&prefered_vector_width_float,
				nullptr			);			assert(success == CL_SUCCESS);			printf("Prefered Vector Width Float : %u", prefered_vector_width_float);			// Get the max clock frequency			cl_uint max_clock_frequency;			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_MAX_CLOCK_FREQUENCY,
				sizeof(cl_uint),
				&max_clock_frequency,
				nullptr			);			assert(success == CL_SUCCESS);			printf("Max Clock Frequency : %u", max_clock_frequency);			// Get the max compute units			cl_uint max_compute_units;			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_MAX_CLOCK_FREQUENCY,
				sizeof(cl_uint),
				&max_compute_units,
				nullptr			);			assert(success == CL_SUCCESS);			printf("Max Compute Units : %u", max_compute_units);			// Get the max work item sizes			cl_uint max_compute_units;			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_MAX_WORK_ITEM_SIZES,
				0,
				nullptr,
				&param_value_size			);			assert(success == CL_SUCCESS);			size_t* max_work_item_sizes = (size_t*)malloc(param_value_size);			success = clGetDeviceInfo(
				devices[device_index],
				CL_DEVICE_MAX_CLOCK_FREQUENCY,
				param_value_size,
				max_work_item_sizes,
				nullptr			);			assert(success == CL_SUCCESS);			printf("Max Work Item Size x : %u", max_work_item_sizes[0]);			printf("Max Work Item Size y : %u", max_work_item_sizes[1]);			printf("Max Work Item Size z : %u", max_work_item_sizes[2]);			// Free up malloc-ed device stuff.			free(device_name);			free(max_work_item_sizes);		}		// Free up malloc-ed platform things.		free(platform_name);		free(devices);	}	// Free up platforms.	free(platforms);}