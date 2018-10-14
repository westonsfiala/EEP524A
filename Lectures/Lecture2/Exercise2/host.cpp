// Add you host code
#include "CL/cl.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>
#include <vector>
#include <thread>

#include "read_source.h"

#define DONT_PRINT_CALL_SUCCESSES

const static std::string platform_name_to_use = "Intel(R) OpenCL";
const static std::string device_name_to_use = "Intel(R) UHD Graphics 620";

const static std::string program_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture2/Exercise2/vecadd_anyD.cl";
const static std::string program_kernel_name = "vecadd_anyD";

/**
 * \brief Checks the result of the output array against the expected output of the input array.
 * \param input_pointer Pointer to the start of the input array.
 * \param output_pointer Pointer to the start of the output array.
 * \param num_values Number of values in the input and output arrays.
 * \return If the output values match what is expected.
 */
bool verifyResults(const float* input_pointer, const float* output_pointer, const int num_values)
{
    assert(num_values > 0);

    printf("Running verification on %d values.\n", num_values);
    auto success = true;

    for (auto i = 0; i < num_values; ++i)
    {
        const auto expected_output = input_pointer[i] * 2;
        const auto output_val = output_pointer[i];
        if(output_val != expected_output)
        {
            printf("Verification failed at index %d. Expected: %f, Got: %f\n", i, expected_output, output_val);
            success = false;
        }
    }

    if(success)
    {
        printf("Verification Success: %d values tested.\n", num_values);
    }
    else
    {
        printf("Verification Fail: %d values tested.\n", num_values);
    }
    return success;
}

/**
 * \brief Frees up all of the pointers contained in the vectors.
 * \param unaligned_pointers Vector to all the pointers that were generated with malloc.
 * \param aligned_pointers Vector to all the pointers that were generated with alligned_malloc.
 */
void free_pointers(const std::vector<void*> &unaligned_pointers, const std::vector<void*> &aligned_pointers)
{
    for(auto unaligned_pointer : unaligned_pointers)
    {
        free(unaligned_pointer);
    }

    for(auto aligned_pointer : aligned_pointers)
    {
        _aligned_free(aligned_pointer);
    }
}

/*
* Given a cl code and return a string represenation
* https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
*/
const char* clGetErrorString(int errorCode) {
    switch (errorCode) {
    case 0: return "CL_SUCCESS";
    case -1: return "CL_DEVICE_NOT_FOUND";
    case -2: return "CL_DEVICE_NOT_AVAILABLE";
    case -3: return "CL_COMPILER_NOT_AVAILABLE";
    case -4: return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
    case -5: return "CL_OUT_OF_RESOURCES";
    case -6: return "CL_OUT_OF_HOST_MEMORY";
    case -7: return "CL_PROFILING_INFO_NOT_AVAILABLE";
    case -8: return "CL_MEM_COPY_OVERLAP";
    case -9: return "CL_IMAGE_FORMAT_MISMATCH";
    case -10: return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
    case -12: return "CL_MAP_FAILURE";
    case -13: return "CL_MISALIGNED_SUB_BUFFER_OFFSET";
    case -14: return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";
    case -15: return "CL_COMPILE_PROGRAM_FAILURE";
    case -16: return "CL_LINKER_NOT_AVAILABLE";
    case -17: return "CL_LINK_PROGRAM_FAILURE";
    case -18: return "CL_DEVICE_PARTITION_FAILED";
    case -19: return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";
    case -30: return "CL_INVALID_VALUE";
    case -31: return "CL_INVALID_DEVICE_TYPE";
    case -32: return "CL_INVALID_PLATFORM";
    case -33: return "CL_INVALID_DEVICE";
    case -34: return "CL_INVALID_CONTEXT";
    case -35: return "CL_INVALID_QUEUE_PROPERTIES";
    case -36: return "CL_INVALID_COMMAND_QUEUE";
    case -37: return "CL_INVALID_HOST_PTR";
    case -38: return "CL_INVALID_MEM_OBJECT";
    case -39: return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
    case -40: return "CL_INVALID_IMAGE_SIZE";
    case -41: return "CL_INVALID_SAMPLER";
    case -42: return "CL_INVALID_BINARY";
    case -43: return "CL_INVALID_BUILD_OPTIONS";
    case -44: return "CL_INVALID_PROGRAM";
    case -45: return "CL_INVALID_PROGRAM_EXECUTABLE";
    case -46: return "CL_INVALID_KERNEL_NAME";
    case -47: return "CL_INVALID_KERNEL_DEFINITION";
    case -48: return "CL_INVALID_KERNEL";
    case -49: return "CL_INVALID_ARG_INDEX";
    case -50: return "CL_INVALID_ARG_VALUE";
    case -51: return "CL_INVALID_ARG_SIZE";
    case -52: return "CL_INVALID_KERNEL_ARGS";
    case -53: return "CL_INVALID_WORK_DIMENSION";
    case -54: return "CL_INVALID_WORK_GROUP_SIZE";
    case -55: return "CL_INVALID_WORK_ITEM_SIZE";
    case -56: return "CL_INVALID_GLOBAL_OFFSET";
    case -57: return "CL_INVALID_EVENT_WAIT_LIST";
    case -58: return "CL_INVALID_EVENT";
    case -59: return "CL_INVALID_OPERATION";
    case -60: return "CL_INVALID_GL_OBJECT";
    case -61: return "CL_INVALID_BUFFER_SIZE";
    case -62: return "CL_INVALID_MIP_LEVEL";
    case -63: return "CL_INVALID_GLOBAL_WORK_SIZE";
    case -64: return "CL_INVALID_PROPERTY";
    case -65: return "CL_INVALID_IMAGE_DESCRIPTOR";
    case -66: return "CL_INVALID_COMPILER_OPTIONS";
    case -67: return "CL_INVALID_LINKER_OPTIONS";
    case -68: return "CL_INVALID_DEVICE_PARTITION_COUNT";
    case -69: return "CL_INVALID_PIPE_SIZE";
    case -70: return "CL_INVALID_DEVICE_QUEUE";
    case -71: return "CL_INVALID_SPEC_ID";
    case -72: return "CL_MAX_SIZE_RESTRICTION_EXCEEDED";
    case -1002: return "CL_INVALID_D3D10_DEVICE_KHR";
    case -1003: return "CL_INVALID_D3D10_RESOURCE_KHR";
    case -1004: return "CL_D3D10_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1005: return "CL_D3D10_RESOURCE_NOT_ACQUIRED_KHR";
    case -1006: return "CL_INVALID_D3D11_DEVICE_KHR";
    case -1007: return "CL_INVALID_D3D11_RESOURCE_KHR";
    case -1008: return "CL_D3D11_RESOURCE_ALREADY_ACQUIRED_KHR";
    case -1009: return "CL_D3D11_RESOURCE_NOT_ACQUIRED_KHR";
    case -1010: return "CL_INVALID_DX9_MEDIA_ADAPTER_KHR";
    case -1011: return "CL_INVALID_DX9_MEDIA_SURFACE_KHR";
    case -1012: return "CL_DX9_MEDIA_SURFACE_ALREADY_ACQUIRED_KHR";
    case -1013: return "CL_DX9_MEDIA_SURFACE_NOT_ACQUIRED_KHR";
    case -1093: return "CL_INVALID_EGL_OBJECT_KHR";
    case -1092: return "CL_EGL_RESOURCE_NOT_ACQUIRED_KHR";
    case -1001: return "CL_PLATFORM_NOT_FOUND_KHR";
    case -1057: return "CL_DEVICE_PARTITION_FAILED_EXT";
    case -1058: return "CL_INVALID_PARTITION_COUNT_EXT";
    case -1059: return "CL_INVALID_PARTITION_NAME_EXT";
    case -1094: return "CL_INVALID_ACCELERATOR_INTEL";
    case -1095: return "CL_INVALID_ACCELERATOR_TYPE_INTEL";
    case -1096: return "CL_INVALID_ACCELERATOR_DESCRIPTOR_INTEL";
    case -1097: return "CL_ACCELERATOR_TYPE_NOT_SUPPORTED_INTEL";
    case -1000: return "CL_INVALID_GL_SHAREGROUP_REFERENCE_KHR";
    case -1098: return "CL_INVALID_VA_API_MEDIA_ADAPTER_INTEL";
    case -1099: return "CL_INVALID_VA_API_MEDIA_SURFACE_INTEL";
    case -1100: return "CL_VA_API_MEDIA_SURFACE_ALREADY_ACQUIRED_INTEL";
    case -1101: return "CL_VA_API_MEDIA_SURFACE_NOT_ACQUIRED_INTEL";
    default: return "CL_UNKNOWN_ERROR";
    }
}

bool process_cl_call_status(const std::string &call_name, const cl_int error_code)
{
    // Success Case
    if(error_code == CL_SUCCESS)
    {
#ifndef DONT_PRINT_CALL_SUCCESSES
        printf("%s(...) succeeded.\n", call_name.c_str());
#endif
        return true;
    }
    // Everything else is a failure.

    printf("%s(...) failed with code: %s\n", call_name.c_str(), clGetErrorString(error_code));
    return false;
}

int main(int argc, char** argv)
{
    // Create containers to track all the pointers than need to be freed when we finish up.
    std::vector<void*> malloced_pointers;
    std::vector<void*> alligned_malloced_pointers;

    cl_uint num_platforms = 0;

    // Get the number of platforms
    auto success = clGetPlatformIDs(
        0,
        nullptr,
        &num_platforms
    );

    if(!process_cl_call_status("clGetPlatformIDs",success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }


    // ReSharper disable once CppLocalVariableMayBeConst
    auto platforms = static_cast<cl_platform_id*>(malloc(sizeof(cl_platform_id) * num_platforms));

    malloced_pointers.push_back(platforms);

    // Get the platforms
    success = clGetPlatformIDs(
        num_platforms,
        platforms,
        nullptr
    );
    if (!process_cl_call_status("clGetPlatformIDs", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }


    cl_platform_id chosen_platform = nullptr;
    cl_device_id* chosen_devices = nullptr;
    cl_uint num_chosen_devices = -1;

    for (cl_uint platform_index = 0; platform_index < num_platforms; platform_index++)
    {
        bool take_platform = true;
        size_t param_value_size;

        // Get the size of the name of the platform.
        success = clGetPlatformInfo(
            platforms[platform_index],
            CL_PLATFORM_NAME,
            0,
            nullptr,
            &param_value_size
        );
        if (!process_cl_call_status("clGetPlatformInfo", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        // ReSharper disable once CppLocalVariableMayBeConst
        auto platform_name = static_cast<char*>(malloc(sizeof(char) * param_value_size));
        malloced_pointers.push_back(platform_name);

        success = clGetPlatformInfo(
            platforms[platform_index],
            CL_PLATFORM_NAME,
            param_value_size,
            platform_name,
            nullptr
        );
        if (!process_cl_call_status("clGetPlatformInfo", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        auto plat_name = std::string(platform_name) + std::string("\n");
        printf("Platform: %s\n", plat_name.c_str());

        if (platform_name != platform_name_to_use)
        {
            take_platform = false;
        }

        cl_uint num_devices;

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platform_index],
            CL_DEVICE_TYPE_ALL,
            0,
            nullptr,
            &num_devices
        );
        if (!process_cl_call_status("clGetDeviceIDs", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        cl_device_id* devices = static_cast<cl_device_id*>(malloc(sizeof(cl_device_id) * num_devices));
        malloced_pointers.push_back(devices);

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platform_index],
            CL_DEVICE_TYPE_ALL,
            num_devices,
            devices,
            &num_devices
        );
        if (!process_cl_call_status("clGetDeviceIDs", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        for (cl_uint device_index = 0; device_index < num_devices; device_index++)
        {
            bool take_device = true;
            // Get info from the devices

            // Get the device name
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_NAME,
                0,
                nullptr,
                &param_value_size
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            // ReSharper disable once CppLocalVariableMayBeConst
            auto device_name = static_cast<char*>(malloc(param_value_size));
            malloced_pointers.push_back(device_name);
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_NAME,
                param_value_size,
                device_name,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            auto dev_name = std::string(device_name) + std::string("\n");
            printf("%s", dev_name.c_str());

            if (device_name != device_name_to_use)
            {
                take_device = false;
            }

            // Get the double config
            cl_device_fp_config fp_config;
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_DOUBLE_FP_CONFIG,
                sizeof(cl_device_fp_config),
                &fp_config,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Double FP Config : %X\n", static_cast<int>(fp_config));

            // Get the preferred vector width float
            cl_uint prefered_vector_width_float;
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                sizeof(cl_uint),
                &prefered_vector_width_float,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Prefered Vector Width Float : %u\n", prefered_vector_width_float);

            // Get the preferred vector width float
            cl_uint native_vector_width_float;
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,
                sizeof(cl_uint),
                &native_vector_width_float,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Native Vector Width Float : %u\n", native_vector_width_float);

            // Get the max clock frequency
            cl_uint max_clock_frequency;
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_CLOCK_FREQUENCY,
                sizeof(cl_uint),
                &max_clock_frequency,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Clock Frequency : %u\n", max_clock_frequency);

            // Get the max compute units
            cl_uint max_compute_units;
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_COMPUTE_UNITS,
                sizeof(cl_uint),
                &max_compute_units,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Compute Units : %u\n", max_compute_units);

            // Get the max work item sizes
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                0,
                nullptr,
                &param_value_size
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            size_t* max_work_item_sizes = static_cast<size_t*>(malloc(param_value_size));
            malloced_pointers.push_back(max_work_item_sizes);
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                param_value_size,
                max_work_item_sizes,
                nullptr
            );
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Work Item Sizes:\n");
            printf("\tMax Work Item Size x : %llu\n", static_cast<uint64_t>(max_work_item_sizes[0]));
            printf("\tMax Work Item Size y : %llu\n", static_cast<uint64_t>(max_work_item_sizes[1]));
            printf("\tMax Work Item Size z : %llu\n", static_cast<uint64_t>(max_work_item_sizes[2]));

            // clean up the output.
            printf("\n");

            if(take_platform && take_device)
            {
                // Select the platform & device that we are using by assigning it here.
                chosen_platform = platforms[platform_index];
                chosen_devices = devices;
                num_chosen_devices = num_devices;
            }
        }
    }

    cl_context_properties properties[3] = { CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(chosen_platform), 0 };

    // Create the context for our run.
    const auto chosen_context = clCreateContext(
        properties,
        num_chosen_devices,
        chosen_devices,
        nullptr,
        nullptr,
        &success
    );
    if (!process_cl_call_status("clCreateContext", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Read in the source file
    //const char* strings[] = {output_global_id.c_str()};

    //const size_t kernel_char_size = output_global_id.size();

    size_t kernel_char_size;

    const auto program_string = read_source( program_file_name.c_str() , &kernel_char_size );

    assert(program_string);

    std::string check_string = program_string;

    const char* strings[] = { program_string };

    malloced_pointers.push_back(program_string);

    // Create the program
    const auto chosen_program = clCreateProgramWithSource(
        chosen_context,
        1,
        strings,
        &kernel_char_size,
        &success
    );
    if (!process_cl_call_status("clCreateContext", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Build the program
    success = clBuildProgram(
        chosen_program,
        num_chosen_devices,
        chosen_devices,
        "-cl-std=CL2.0",
        nullptr,
        nullptr
    );
    if (!process_cl_call_status("clCreateContext", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Create the command queue
    const auto chosen_command_queue = clCreateCommandQueueWithProperties(
        chosen_context,
        chosen_devices[0],
        nullptr,
        &success
    );
    if (!process_cl_call_status("clCreateCommandQueueWithProperties", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Create the Kernel
    const auto chosen_kernel = clCreateKernel(
        chosen_program,
        program_kernel_name.c_str(),
        &success
    );
    if (!process_cl_call_status("clCreateKernel", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Allocate memory for the input.
    const auto alignment = 4096;
    const auto num_values = 8192;
    const size_t chosen_memory_size = sizeof(float) * num_values;
    void* chosen_input_pointer = _aligned_malloc(sizeof(float) * chosen_memory_size, alignment);

    if(chosen_input_pointer)
    {
        alligned_malloced_pointers.push_back(chosen_input_pointer);
    }

    // Initialize the inputs to linearly increasing index.
    for(auto i = 1; i <= num_values; ++i)
    {
        static_cast<float*>(chosen_input_pointer)[i] = static_cast<float>(i);
    }

    // Create the memory buffers
    auto chosen_input_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        chosen_memory_size,
        chosen_input_pointer,
        &success
    );
    if (!process_cl_call_status("clCreateBuffer", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Allocate memory for the output.
    void* chosen_output_pointer = _aligned_malloc(sizeof(float) * chosen_memory_size, num_values);

    if (chosen_output_pointer)
    {
        alligned_malloced_pointers.push_back(chosen_output_pointer);
    }

    // Create the memory buffers
    auto chosen_output_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        chosen_memory_size,
        chosen_output_pointer,
        &success
    );
    if (!process_cl_call_status("clCreateBuffer", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Set the kernel arguments
    // Input A
    success = clSetKernelArg(
        chosen_kernel,
        0,
        sizeof(chosen_input_buffer),
        &chosen_input_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Input B
    success = clSetKernelArg(
        chosen_kernel,
        1,
        sizeof(chosen_input_buffer),
        &chosen_input_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Output
    success = clSetKernelArg(
        chosen_kernel,
        2,
        sizeof(chosen_output_buffer),
        &chosen_output_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Enque the kernel
    size_t global_work_offset[] = {0, 0, 0};
    size_t global_work_size[] = {num_values, 0, 0};
    size_t local_work_size[] = {256, 256, 256};

    success = clEnqueueNDRangeKernel(
        chosen_command_queue,
        chosen_kernel,
        1,
        global_work_offset,
        global_work_size,
        local_work_size,
        0,
        nullptr,
        nullptr
    );
    if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Wait for the kernel to finish
    success = clFinish(chosen_command_queue);
    if (!process_cl_call_status("clFinish", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Read back the data that we processed
    const auto chosen_kernel_output = clEnqueueMapBuffer(
        chosen_command_queue,
        chosen_output_buffer,
        true,
        CL_MAP_READ,
        0,
        chosen_memory_size,
        0,
        nullptr,
        nullptr,
        &success
    );
    if (!process_cl_call_status("clEnqueueMapBuffer", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Free the memory that we mapped before - Output
    success = clEnqueueUnmapMemObject(
        chosen_command_queue,
        chosen_output_buffer,
        chosen_output_pointer,
        0,
        nullptr,
        nullptr
    );
    if (!process_cl_call_status("clEnqueueUnmapMemObject", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Verify our relults
    success = verifyResults(static_cast<float*>(chosen_input_pointer), static_cast<float*>(chosen_kernel_output), num_values);
    assert(success);

    // Free up all of the pointers that we allocated earlier.
    free_pointers(malloced_pointers, alligned_malloced_pointers);

    printf("\nsleeping for 5 seconds\n");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
