// Add you host code
#include "CL/cl.h"
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <string>
#include <vector>
#include <fstream>
#include <thread>
#include <Windows.h>

#include "read_source.h"

#define DONT_PRINT_CALL_SUCCESSES

const static std::string platform_name_to_use = "Intel(R) OpenCL";
// const static std::string device_name_to_use = "Intel(R) UHD Graphics 620";
const static std::string device_name_to_use = "Intel(R) Core(TM) i5-8250U CPU @ 1.60GHz";

const static std::string program_file_name = "C:/work/GitHub/EEP524A/Homeworks/Homework3/mmul_opt1.cl";
const static std::string naive_kernel_name = "mmul_v1";
const static std::string opt1_kernel_name = "mmul_v2";

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
        if (output_val != expected_output)
        {
            printf("Verification failed at index %d. Expected: %f, Got: %f\n", i, expected_output, output_val);
            success = false;
        }
    }

    if (success)
    {
        printf("Verification Success: %d values tested.\n", num_values);
    }
    else
    {
        printf("Verification Fail: %d values tested.\n", num_values);
    }
    return success;
}


static const std::string output_directory = "C:/work/GitHub/EEP524A/Homeworks/Homework3/";

// Got from https://stackoverflow.com/questions/33268513/calculating-standard-deviation-variance-in-c
double Variance(const std::vector<double>& samples)
{
    const auto size = samples.size();

    double variance = 0;
    double t = samples[0];
    for (auto i = 1; i < size; i++)
    {
        t += samples[i];
        const double diff = ((i + 1) * samples[i]) - t;
        variance += (diff * diff) / ((i + 1.0) * i);
    }

    return variance / (size - 1);
}

double StandardDeviation(const std::vector<double>& samples)
{
    return sqrt(Variance(samples));
}

double Average(const std::vector<double>& samples)
{
    auto average = 0.0;
    if (!samples.empty())
    {
        for (auto sample : samples)
        {
            average += sample;
        }
        average /= samples.size();
    }

    return average;
}

std::pair<double, double> print_results(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times,
                                        const std::string& filename)
{
    std::ofstream myfile;

    const auto file_path = output_directory + filename + ".csv";

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    std::vector<double> samples;

    myfile.open(file_path, std::ios::out);

    for (const auto& time_stamp : times)
    {
        double elapsed_time = time_stamp.second.QuadPart - time_stamp.first.QuadPart;

        // Convert from number of counts to number of milliseconds
        elapsed_time *= 1000000;
        elapsed_time /= frequency.QuadPart;

        samples.push_back(static_cast<double>(elapsed_time));

        myfile << elapsed_time << std::endl;
    }

    myfile.close();

    return {Average(samples), StandardDeviation(samples)};
}

std::pair<double, double> print_results(std::vector<std::pair<cl_ulong, cl_ulong>>& times, const std::string& filename)
{
    std::ofstream myfile;

    const auto file_path = output_directory + filename + ".csv";

    std::vector<double> samples;

    myfile.open(file_path, std::ios::out);

    for (const auto& time_stamp : times)
    {
        double elapsed_time = time_stamp.second - time_stamp.first;

        elapsed_time /= 1000000.0;

        samples.push_back(elapsed_time);

        myfile << elapsed_time << std::endl;
    }

    myfile.close();
    return {Average(samples), StandardDeviation(samples)};
}


/**
 * \brief Frees up all of the pointers contained in the vectors.
 * \param unaligned_pointers Vector to all the pointers that were generated with malloc.
 * \param aligned_pointers Vector to all the pointers that were generated with alligned_malloc.
 */
void free_pointers(std::vector<void*>& unaligned_pointers, std::vector<void*>& aligned_pointers)
{
    for (auto unaligned_pointer : unaligned_pointers)
    {
        free(unaligned_pointer);
    }

    for (auto aligned_pointer : aligned_pointers)
    {
        _aligned_free(aligned_pointer);
    }

    unaligned_pointers.clear();
    aligned_pointers.clear();
}

/*
* Given a cl code and return a string represenation
* https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
*/
const char* clGetErrorString(int errorCode)
{
    switch (errorCode)
    {
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

bool process_cl_call_status(const std::string& call_name, const cl_int error_code)
{
    // Success Case
    if (error_code == CL_SUCCESS)
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

    if (!process_cl_call_status("clGetPlatformIDs", success))
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

            if (take_platform && take_device)
            {
                // Select the platform & device that we are using by assigning it here.
                chosen_platform = platforms[platform_index];
                chosen_devices = devices;
                num_chosen_devices = num_devices;
            }
        }
    }

    cl_context_properties properties[3] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(chosen_platform), 0
    };

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

    size_t kernel_char_size;

    const auto program_string = read_source(program_file_name.c_str(), &kernel_char_size);

    assert(program_string);

    std::string check_string = program_string;

    const char* strings[] = {program_string};

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

    cl_queue_properties queue_properties[3] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};

    // Create the command queue
    const auto chosen_command_queue = clCreateCommandQueueWithProperties(
        chosen_context,
        chosen_devices[0],
        queue_properties,
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

    // Create the naive Kernel
    const auto naive_kernel = clCreateKernel(
        chosen_program,
        naive_kernel_name.c_str(),
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

    // Create the opt1 Kernel
    const auto opt1_kernel = clCreateKernel(
        chosen_program,
        opt1_kernel_name.c_str(),
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

    // Assign variables
    // Arg 0 - N

    const auto matrix_size = 512;

    cl_int arg0 = matrix_size;
    success = clSetKernelArg(
        naive_kernel,
        0,
        sizeof(arg0),
        &arg0
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Arg1 - Matrix A
    const auto num_values = matrix_size * matrix_size;
    const auto num_matrix_bytes = sizeof(cl_float) * num_values;
    auto matrix_a_ptr = static_cast<cl_float*>(_aligned_malloc(num_matrix_bytes, matrix_size));

    if (matrix_a_ptr)
    {
        alligned_malloced_pointers.push_back(matrix_a_ptr);
    }

    // Fill up the pointer with useful values
    for (auto i = 0; i < num_values; ++i)
    {
        matrix_a_ptr[i] = static_cast<cl_float>(i);
    }

    auto matrix_a_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        num_matrix_bytes,
        matrix_a_ptr,
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

    success = clSetKernelArg(
        naive_kernel,
        1,
        sizeof(matrix_a_buffer),
        &matrix_a_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Argument 2 - Matrix B
    auto matrix_b_ptr = static_cast<cl_float*>(_aligned_malloc(num_matrix_bytes, matrix_size));

    if (matrix_b_ptr)
    {
        alligned_malloced_pointers.push_back(matrix_b_ptr);
    }

    // Fill up the pointer with useful values
    for (auto i = 0; i < num_values; ++i)
    {
        matrix_b_ptr[i] = static_cast<cl_float>(num_values - i);
    }

    auto matrix_b_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        num_matrix_bytes,
        matrix_b_ptr,
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

    success = clSetKernelArg(
        naive_kernel,
        2,
        sizeof(matrix_b_buffer),
        &matrix_b_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Argument 3 - Matrix output C
    auto matrix_c_ptr = _aligned_malloc(num_matrix_bytes, matrix_size);

    if (matrix_c_ptr)
    {
        alligned_malloced_pointers.push_back(matrix_c_ptr);
    }

    // Create the memory buffers
    auto matrix_c_buffer = clCreateBuffer(
        chosen_context,
        CL_MEM_USE_HOST_PTR,
        num_matrix_bytes,
        matrix_c_ptr,
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
        naive_kernel,
        3,
        sizeof(matrix_c_buffer),
        &matrix_c_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Set Arguments for the opt1 kernel
    success = clSetKernelArg(
        opt1_kernel,
        0,
        sizeof(arg0),
        &arg0
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Arg1 - Matrix A

    success = clSetKernelArg(
        opt1_kernel,
        1,
        sizeof(matrix_a_buffer),
        &matrix_a_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Argument 2 - Matrix B

    success = clSetKernelArg(
        opt1_kernel,
        2,
        sizeof(matrix_b_buffer),
        &matrix_b_buffer
    );
    if (!process_cl_call_status("clSetKernelArg", success))
    {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Argument 3 - Matrix output C

    // Set the kernel arguments
    // Input A
    success = clSetKernelArg(
        opt1_kernel,
        3,
        sizeof(matrix_c_buffer),
        &matrix_c_buffer
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
    size_t global_work_size[] = {512, 512, 0};
    size_t local_work_size[] = {1, 64, 0};

    std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> naive_time_captures_enque;
    std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> naive_time_captures_finish;
    std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> opt1_time_captures_enque;
    std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> opt1_time_captures_finish;

    std::vector<std::pair<cl_ulong, cl_ulong>> cl_naive_time_captures_submit;
    std::vector<std::pair<cl_ulong, cl_ulong>> cl_naive_time_captures_finish;
    std::vector<std::pair<cl_ulong, cl_ulong>> cl_opt1_time_captures_submit;
    std::vector<std::pair<cl_ulong, cl_ulong>> cl_opt1_time_captures_finish;

    LARGE_INTEGER start_time, finish_time;
    cl_ulong cl_submit_time, cl_start_time, cl_finish_time;

    printf("Starting naive enque\n");
    for (auto i = 0; i < 500; ++i)
    {
        // Captures for the naive enque time stamps.
        QueryPerformanceCounter(&start_time);

        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            naive_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            nullptr
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }
        */

        QueryPerformanceCounter(&finish_time);

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
            free_pointers(malloced_pointers, alligned_malloced_pointers);
            // In debug its nice to hit an assert so that we stop and
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }
        */

        //QueryPerformanceCounter(&finish_time);

        naive_time_captures_enque.emplace_back(start_time, finish_time);
    }

    printf("Starting naive finish\n");
    for (int i = 0; i < 500; ++i)
    {
        // Captures for the naive finsh time stamps.
        QueryPerformanceCounter(&start_time);

        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            naive_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            nullptr
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        //QueryPerformanceCounter(&finish_time);

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        QueryPerformanceCounter(&finish_time);

        naive_time_captures_finish.emplace_back(start_time, finish_time);
    }

    printf("Starting naive submit & finish cl\n");
    for (auto i = 0; i < 500; ++i)
    {
        // Captures for the naive enque time stamps.
        cl_event capture_event;
        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            naive_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            &capture_event
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */


        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_SUBMIT,
            sizeof(cl_ulong),
            &cl_submit_time,
            nullptr
        );

        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_START,
            sizeof(cl_ulong),
            &cl_start_time,
            nullptr
        );

        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_END,
            sizeof(cl_ulong),
            &cl_finish_time,
            nullptr
        );

        cl_naive_time_captures_submit.emplace_back(cl_submit_time, cl_finish_time);
        cl_naive_time_captures_finish.emplace_back(cl_start_time, cl_finish_time);
    }

    printf("Starting opt1 enque\n");
    for (int i = 0; i < 500; ++i)
    {
        // Captures for the opt1 enque time stamps
        QueryPerformanceCounter(&start_time);

        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            opt1_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            nullptr
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        QueryPerformanceCounter(&finish_time);

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        //QueryPerformanceCounter(&finish_time);

        opt1_time_captures_enque.emplace_back(start_time, finish_time);
    }

    printf("Starting opt1 finish\n");
    for (int i = 0; i < 500; ++i)
    {
        // Captures for the opt1 finish time stamps
        QueryPerformanceCounter(&start_time);

        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            opt1_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            nullptr
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        //QueryPerformanceCounter(&finish_time);

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        QueryPerformanceCounter(&finish_time);

        opt1_time_captures_finish.emplace_back(start_time, finish_time);
    }

    printf("Starting opt1 submit & finish cl\n");
    for (auto i = 0; i < 500; ++i)
    {
        // Captures for the naive enque time stamps.
        cl_event capture_event;
        success = clEnqueueNDRangeKernel(
            chosen_command_queue,
            opt1_kernel,
            2,
            global_work_offset,
            global_work_size,
            local_work_size,
            0,
            nullptr,
            &capture_event
        );
        /*
        if (!process_cl_call_status("clEnqueueNDRangeKernel", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */

        // Wait for the kernel to finish
        success = clFinish(chosen_command_queue);
        /*
        if (!process_cl_call_status("clFinish", success))
        {
        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
        }
        */


        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_SUBMIT,
            sizeof(cl_ulong),
            &cl_submit_time,
            nullptr
        );

        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_START,
            sizeof(cl_ulong),
            &cl_start_time,
            nullptr
        );

        clGetEventProfilingInfo(
            capture_event,
            CL_PROFILING_COMMAND_END,
            sizeof(cl_ulong),
            &cl_finish_time,
            nullptr
        );

        cl_opt1_time_captures_submit.emplace_back(cl_submit_time, cl_finish_time);
        cl_opt1_time_captures_finish.emplace_back(cl_start_time, cl_finish_time);
    }

    /*
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
    */

    // Verify our relults
    //success = verifyResults(static_cast<float*>(chosen_input_pointer), static_cast<float*>(chosen_kernel_output), num_values);
    //assert(success);

    // Free up all of the pointers that we allocated earlier.
    free_pointers(malloced_pointers, alligned_malloced_pointers);

    const auto naive_enque_results = print_results(naive_time_captures_enque, "naive_enque");
    const auto naive_finish_results = print_results(naive_time_captures_finish, "naive_finish");
    const auto opt1_enque_results = print_results(opt1_time_captures_enque, "opt1_enque");
    const auto opt1_finish_results = print_results(opt1_time_captures_finish, "opt1_finish");

    const auto cl_naive_submit_results = print_results(cl_naive_time_captures_submit, "cl_naive_submit");
    const auto cl_naive_finish_results = print_results(cl_naive_time_captures_finish, "cl_naive_finish");
    const auto cl_opt1_submit_results = print_results(cl_opt1_time_captures_submit, "cl_opt1_submit");
    const auto cl_opt1_finish_results = print_results(cl_opt1_time_captures_finish, "cl_opt1_finish");


    std::ofstream myfile;

    myfile.open(output_directory + "result_output.txt", std::ios::out);

    myfile << "naive_enque: Average = " << naive_enque_results.first << "ms; Standard Deviation = " <<
        naive_enque_results.second << std::endl;
    myfile << "naive_finish: Average = " << naive_finish_results.first << "ms; Standard Deviation = " <<
        naive_finish_results.second << std::endl;
    myfile << "opt1_enque: Average = " << opt1_enque_results.first << "ms; Standard Deviation = " << opt1_enque_results.
        second << std::endl;
    myfile << "opt1_finish: Average = " << opt1_finish_results.first << "ms; Standard Deviation = " <<
        opt1_finish_results.second << std::endl;
    myfile << "cl_naive_submit: Average = " << cl_naive_submit_results.first << "ms; Standard Deviation = " <<
        cl_naive_submit_results.second << std::endl;
    myfile << "cl_naive_finish: Average = " << cl_naive_finish_results.first << "ms; Standard Deviation = " <<
        cl_naive_finish_results.second << std::endl;
    myfile << "cl_opt1_submit: Average = " << cl_opt1_submit_results.first << "ms; Standard Deviation = " <<
        cl_opt1_submit_results.second << std::endl;
    myfile << "cl_opt1_finish: Average = " << cl_opt1_finish_results.first << "ms; Standard Deviation = " <<
        cl_opt1_finish_results.second << std::endl;

    myfile.close();

    return 0;
}
