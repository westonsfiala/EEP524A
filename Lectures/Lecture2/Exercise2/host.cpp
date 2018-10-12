// Add you host code
#include "CL/cl.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>
#include <vector>
#include <thread>

#include "read_source.h"

const static std::string platform_name_to_use = "Intel(R) OpenCL";
const static std::string device_name_to_use = "Intel(R) UHD Graphics 620";

const static std::string program_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture2/Exercise2/vecadd_anyD.cl";
const static std::string program_kernel_name = "vecadd_anyD";

bool verifyResults(const float* input_pointer, const float* output_pointer, const int num_values)
{
    assert(num_values > 0);

    auto success = true;

    for (auto i = 0; i < num_values; ++i)
    {
        const auto input_val = input_pointer[i];
        const auto output_val = output_pointer[i];
        success &= output_val == input_val*2;
    }

    if(success)
    {
        printf("Verification Success\n");
    }
    else
    {
        printf("Verification Fail\n");
    }
    return success;
}

int main(int argc, char** argv)
{
    cl_uint num_platforms = 0;

    // Get the number of platforms
    auto success = clGetPlatformIDs(
        0,
        nullptr,
        &num_platforms
    );
    assert(success == CL_SUCCESS);

    std::vector<void*> malloced_pointers;

    // ReSharper disable once CppLocalVariableMayBeConst
    auto platforms = static_cast<cl_platform_id*>(malloc(sizeof(cl_platform_id) * num_platforms));

    malloced_pointers.push_back(platforms);

    // Get the platforms
    success = clGetPlatformIDs(
        num_platforms,
        platforms,
        nullptr
    );
    assert(success == CL_SUCCESS);

    cl_platform_id chosen_platform = nullptr;
    cl_device_id* chosen_devices = nullptr;
    cl_uint num_chosen_devices = -1;

    for (cl_uint platform_index = 0; platform_index < num_platforms; platform_index++)
    {
        size_t param_value_size;

        // Get the size of the name of the platform.
        success = clGetPlatformInfo(
            platforms[platform_index],
            CL_PLATFORM_NAME,
            0,
            nullptr,
            &param_value_size
        );
        assert(success == CL_SUCCESS);

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
        assert(success == CL_SUCCESS);

        auto plat_name = std::string(platform_name) + std::string("\n");
        printf("Platform: %s\n", plat_name.c_str());

        if (platform_name != platform_name_to_use)
        {
            continue;
        }

        cl_uint num_devices;

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platform_index],
            CL_DEVICE_TYPE_GPU,
            0,
            nullptr,
            &num_devices
        );
        assert(success == CL_SUCCESS);

        cl_device_id* devices = static_cast<cl_device_id*>(malloc(sizeof(cl_device_id) * num_devices));
        malloced_pointers.push_back(devices);

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platform_index],
            CL_DEVICE_TYPE_GPU,
            num_devices,
            devices,
            &num_devices
        );
        assert(success == CL_SUCCESS);

        for (cl_uint device_index = 0; device_index < num_devices; device_index++)
        {
            // Get info from the devices

            // Get the device name
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_NAME,
                0,
                nullptr,
                &param_value_size
            );
            assert(success == CL_SUCCESS);
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
            assert(success == CL_SUCCESS);
            auto dev_name = std::string(device_name) + std::string("\n");
            printf("%s", dev_name.c_str());

            if (device_name != device_name_to_use)
            {
                continue;
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
            assert(success == CL_SUCCESS);
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
            assert(success == CL_SUCCESS);
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
            assert(success == CL_SUCCESS);
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
            assert(success == CL_SUCCESS);
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
            assert(success == CL_SUCCESS);
            printf("Max Compute Units : %u\n", max_compute_units);

            // Get the max work item sizes
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                0,
                nullptr,
                &param_value_size
            );
            assert(success == CL_SUCCESS);
            size_t* max_work_item_sizes = static_cast<size_t*>(malloc(param_value_size));
            malloced_pointers.push_back(max_work_item_sizes);
            success = clGetDeviceInfo(
                devices[device_index],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                param_value_size,
                max_work_item_sizes,
                nullptr
            );
            assert(success == CL_SUCCESS);
            printf("Max Work Item Sizes:\n");
            printf("\tMax Work Item Size x : %llu\n", static_cast<uint64_t>(max_work_item_sizes[0]));
            printf("\tMax Work Item Size y : %llu\n", static_cast<uint64_t>(max_work_item_sizes[1]));
            printf("\tMax Work Item Size z : %llu\n", static_cast<uint64_t>(max_work_item_sizes[2]));

            // clean up the output.
            printf("\n");

            // Select the platform & device that we are using by assigning it here.
            chosen_platform = platforms[platform_index];
            chosen_devices = devices;
            num_chosen_devices = num_devices;
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
    assert(success == CL_SUCCESS);

    // Read in the source file
    //const char* strings[] = {output_global_id.c_str()};

    //const size_t kernel_char_size = output_global_id.size();

    size_t kernel_char_size;

    const auto program_string = read_source( program_file_name.c_str() , &kernel_char_size );

    assert(program_string);

    std::string check_string = program_string;

    const char* strings[] = { program_string };

    // Create the program
    const auto chosen_program = clCreateProgramWithSource(
        chosen_context,
        1,
        strings,
        &kernel_char_size,
        &success
    );
    assert(success == CL_SUCCESS);

    // Build the program
    success = clBuildProgram(
        chosen_program,
        num_chosen_devices,
        chosen_devices,
        "-cl-std=CL2.0",
        nullptr,
        nullptr
    );
    assert(success == CL_SUCCESS);

    // Create the command queue
    const auto chosen_command_queue = clCreateCommandQueueWithProperties(
        chosen_context,
        chosen_devices[0],
        nullptr,
        &success
    );
    assert(success == CL_SUCCESS);

    // Create the Kernel
    const auto chosen_kernel = clCreateKernel(
        chosen_program,
        program_kernel_name.c_str(),
        &success
    );
    assert(success == CL_SUCCESS);

    // Allocate memory for the input.
    const auto num_values = 4096;
    const size_t chosen_memory_size = sizeof(float) * num_values;
    void* chosen_input_pointer = _aligned_malloc(sizeof(float) * chosen_memory_size, num_values);

    // Initialize the inputs to linearly increasing index.
    for(auto i = 0; i < num_values; ++i)
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
    assert(success == CL_SUCCESS);

    // Allocate memory for the output.
    void* chosen_output_pointer = _aligned_malloc(sizeof(float) * chosen_memory_size, num_values);

    // Create the memory buffers
    auto chosen_output_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        chosen_memory_size,
        chosen_output_pointer,
        &success
    );
    assert(success == CL_SUCCESS);

    // Set the kernel arguments
    // Input A
    success = clSetKernelArg(
        chosen_kernel,
        0,
        sizeof(chosen_input_buffer),
        &chosen_input_buffer
    );
    assert(success == CL_SUCCESS);

    // Input B
    success = clSetKernelArg(
        chosen_kernel,
        1,
        sizeof(chosen_input_buffer),
        &chosen_input_buffer
    );
    assert(success == CL_SUCCESS);

    // Output
    success = clSetKernelArg(
        chosen_kernel,
        2,
        sizeof(chosen_output_buffer),
        &chosen_output_buffer
    );
    assert(success == CL_SUCCESS);

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
    assert(success == CL_SUCCESS);

    // Wait for the kernel to finish
    success = clFinish(chosen_command_queue);
    assert(success == CL_SUCCESS);

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
    assert(success == CL_SUCCESS);

    // Free the memory that we mapped before - Output
    success = clEnqueueUnmapMemObject(
        chosen_command_queue,
        chosen_output_buffer,
        chosen_output_pointer,
        0,
        nullptr,
        nullptr
    );
    assert(success == CL_SUCCESS);

    // Verify our relults
    success = verifyResults(static_cast<float*>(chosen_input_pointer), static_cast<float*>(chosen_kernel_output), num_values);
    assert(success);

    // Free up the aligned memory.
    _aligned_free(chosen_input_pointer);
    _aligned_free(chosen_output_pointer);

    // Free up any pointers that we malloced earlier.
    for (auto malloced_pointer : malloced_pointers)
    {
        free(malloced_pointer);
    }

    printf("\nsleeping for 5 seconds\n");
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}
