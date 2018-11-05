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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "GenerateCoef.h"
#include <map>

#define DONT_PRINT_CALL_SUCCESSES

// What platform and device to use.
const static std::string platform_name_to_use = "Intel(R) OpenCL";
const static std::string prefered_device_name = "Intel(R) UHD Graphics 620";

/*************************************************************************
 * CHANGE THE FOLLOWING LINES TO WHERE EVER YOU HAVE EVERYTHING MAPPED TO.
 *************************************************************************/
//Set the output directories that need to be used.
static const std::string base_directory = "C:/work/GitHub/EEP524A/Homeworks/Homework4/";
static const std::string output_directory = base_directory + "Outputs/";
static const std::string src_directory = base_directory + "src/";
static const std::string blur_kernel_file = src_directory + "Blur_Kernel.cl";
static const std::string lena_file = src_directory + "tiny-Lena_32bit.png";
//static const std::string little_lena_file = src_directory + "little-Lena_24bit.png";
static const std::string normal_lena_file = src_directory + "lena_512x512_32bit.png";
static const std::string big_lena_file = src_directory + "Big_Gray-Lena_8bit.png";

static const std::vector<std::string> lena_files = { lena_file/*, little_lena_file*/, normal_lena_file, big_lena_file };

const static std::string conv_filter_kernel_name = "img_conv_filter";

const static std::vector<uint32_t> filter_sizes = { 5, 7, 9 };
const static std::vector<float> filter_sigma2s = { 0.75f, 1.2f };

const static uint32_t kernel_iterations = 500;

// Got from https://stackoverflow.com/questions/33268513/calculating-standard-deviation-variance-in-c
double Variance(const std::vector<double>& samples)
{
    const auto size = samples.size();

    double variance = 0;
    double t = samples[0];
    for (uint32_t i = 1; i < size; i++)
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

std::pair<double, double> print_results(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times, const std::string& filename)
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
    case -11: return "CL_BUILD_PROGRAM_FAILURE";
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


    const auto platforms = static_cast<cl_platform_id*>(malloc(sizeof(cl_platform_id) * num_platforms));

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

        const auto platform_name = static_cast<char*>(malloc(sizeof(char) * param_value_size));
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
            CL_DEVICE_TYPE_GPU,
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
            CL_DEVICE_TYPE_GPU,
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

        bool device_taken = false;

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
            if (!process_cl_call_status("clGetDeviceInfo", success))
            {
                free_pointers(malloced_pointers, alligned_malloced_pointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            const auto device_name = static_cast<char*>(malloc(param_value_size));
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
            const std::string device_name_string = device_name;
            auto dev_name = device_name_string + std::string("\n");
            printf("%s", dev_name.c_str());

            if (device_name_string == prefered_device_name)
            {
                device_taken = true;
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

            if (take_platform)
            {
                // Select the platform & device that we are using by assigning it here.
                chosen_platform = platforms[platform_index];
                chosen_devices = devices;
                num_chosen_devices = num_devices;
                // If we have our prefered device, don't keep searching.
                if (device_taken)
                {
                    break;
                }
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

    size_t blur_kernel_char_size;

    const auto blur_kernel_string = read_source(blur_kernel_file.c_str(), &blur_kernel_char_size);

    std::string check_string1 = blur_kernel_string;

    assert(blur_kernel_string);

    // The strings array is filled with the two null terminated strings.
    const char* strings[] = { blur_kernel_string};
    // Because the strings are null terminated, we pass in 0.
    const size_t lengths[] = {0};

    malloced_pointers.push_back(blur_kernel_string);

    // Create the program
    const auto chosen_program = clCreateProgramWithSource(
        chosen_context,
        1,
        strings,
        lengths,
        &success
    );
    if (!process_cl_call_status("clCreateProgramWithSource", success))
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
    if (!process_cl_call_status("clBuildProgram", success))
    {
        // Get detailed build errors when they occur.
        size_t build_error_log_size;
        clGetProgramBuildInfo(chosen_program, chosen_devices[0], CL_PROGRAM_BUILD_LOG, 0, nullptr,
                              &build_error_log_size);

        char* build_error = static_cast<char*>(malloc(sizeof(char) * build_error_log_size));
        malloced_pointers.push_back(build_error);

        clGetProgramBuildInfo(chosen_program, chosen_devices[0], CL_PROGRAM_BUILD_LOG, build_error_log_size,
                              build_error, nullptr);

        std::string error_string = build_error;
        printf("%s", error_string.c_str());

        free_pointers(malloced_pointers, alligned_malloced_pointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    //cl_queue_properties queue_properties[3] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};

    // Create the command queue
    const auto chosen_command_queue = clCreateCommandQueueWithProperties(
        chosen_context,
        chosen_devices[0],
        nullptr, //queue_properties,
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
    const auto blur_kernel = clCreateKernel(
        chosen_program,
        conv_filter_kernel_name.c_str(),
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

    std::map<std::string, std::pair<double, double>> results_outputs;

    for (const auto &lena_file : lena_files)
    {
        for (const auto &filter_size : filter_sizes)
        {
            for (const auto &filter_sigma2 : filter_sigma2s)
            {
                const cl_mem_flags lena_input_image_flags = CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY;
                const cl_mem_flags lena_output_image_flags = CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY;

                /*
                size_t num_supported;

                // Find out what image formats are supported
                success = clGetSupportedImageFormats(
                    chosen_context,
                    lena_output_image_flags,
                    CL_MEM_OBJECT_IMAGE2D,
                    0,
                    nullptr,
                    &num_supported
                );
                if (!process_cl_call_status("clGetSupportedImageFormats", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                cl_image_format* supported_formats = static_cast<cl_image_format*>(malloc(sizeof(cl_image_format) * num_supported));
                malloced_pointers.push_back(supported_formats);

                // Find out what image formats are supported
                success = clGetSupportedImageFormats(
                    chosen_context,
                    lena_output_image_flags,
                    CL_MEM_OBJECT_IMAGE2D,
                    num_supported,
                    supported_formats,
                    nullptr
                );
                if (!process_cl_call_status("clGetSupportedImageFormats", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                for(uint32_t i = 0; i < num_supported; ++i)
                {
                    const auto supported_format = supported_formats[i];
                    if(supported_format.image_channel_data_type == CL_UNSIGNED_INT8)
                    {
                        const auto supported_order = supported_format.image_channel_order;
                        if(supported_order == CL_RGB)
                        {
                            auto blah = 5;
                        }
                    }
                }
                */
                

                // Assign variables
                // Blur Kernel

                // Arg 0 - __read_only image2d_t inputImg
                int32_t lena_x;
                int32_t lena_y;
                int32_t lena_num_channels;
                const auto lena_data = stbi_load(lena_file.c_str(), &lena_x, &lena_y, &lena_num_channels, 0);
                assert(lena_data);
                malloced_pointers.push_back(lena_data);

                cl_image_format lena_image_format;
                lena_image_format.image_channel_data_type = CL_UNSIGNED_INT8;
                switch (lena_num_channels)
                {
                case 1:
                    lena_image_format.image_channel_order = CL_R;
                    break;
                case 2:
                    lena_image_format.image_channel_order = CL_RA;
                    break;
                case 3:
                    lena_image_format.image_channel_order = CL_RGB;
                    break;
                case 4:
                    lena_image_format.image_channel_order = CL_RGBA;
                    break;
                default:
                    assert(false);
                    break;
                }

                cl_image_desc lena_image_desc;
                lena_image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
                lena_image_desc.image_width = lena_x;
                lena_image_desc.image_height = lena_y;
                lena_image_desc.image_depth = 0;
                lena_image_desc.image_array_size = 0;
                lena_image_desc.image_row_pitch = 0;
                lena_image_desc.image_slice_pitch = 0;
                lena_image_desc.num_mip_levels = 0;
                lena_image_desc.num_samples = 0;
                lena_image_desc.mem_object = nullptr;

                auto lena_image_mem = clCreateImage(
                    chosen_context,
                    lena_input_image_flags,
                    &lena_image_format,
                    &lena_image_desc,
                    lena_data,
                    &success
                );
                if (!process_cl_call_status("clCreateImage", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blur_kernel,
                    0,
                    sizeof(lena_image_mem),
                    &lena_image_mem
                );
                if (!process_cl_call_status("clSetKernelArg", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg1 - __write_only image2d_t outputImg,
                const auto lena_output_data = malloc(sizeof(char) * lena_x * lena_y * lena_num_channels);
                malloced_pointers.push_back(lena_output_data);
                assert(lena_data);

                auto lena_output_image_mem = clCreateImage(
                    chosen_context,
                    lena_output_image_flags,
                    &lena_image_format,
                    &lena_image_desc,
                    lena_data,
                    &success
                );
                if (!process_cl_call_status("clCreateImage", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blur_kernel,
                    1,
                    sizeof(lena_output_image_mem),
                    &lena_output_image_mem
                );
                if (!process_cl_call_status("clSetKernelArg", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg2 - sampler_t sampler

                cl_sampler_properties lena_sampler_properties[] = {
                    CL_SAMPLER_NORMALIZED_COORDS, false,
                    CL_SAMPLER_ADDRESSING_MODE, CL_ADDRESS_CLAMP,
                    CL_SAMPLER_FILTER_MODE, CL_FILTER_LINEAR
                    ,0 };

                auto lena_sampler = clCreateSamplerWithProperties(
                    chosen_context,
                    lena_sampler_properties,
                    &success
                );

                if (!process_cl_call_status("clCreateSamplerWithProperties", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blur_kernel,
                    2,
                    sizeof(lena_sampler),
                    &lena_sampler
                );
                if (!process_cl_call_status("clSetKernelArg", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg 3 - __global float* filter

                // Get a vector of all the filter coefs.
                auto blur_filter = generate_gauss_blur(filter_size, filter_sigma2);

                auto blur_filter_mem = clCreateBuffer(
                    chosen_context,
                    CL_MEM_USE_HOST_PTR,
                    sizeof(float) * blur_filter.size(),
                    blur_filter.data(),
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
                    blur_kernel,
                    3,
                    sizeof(blur_filter_mem),
                    &blur_filter_mem
                );
                if (!process_cl_call_status("clSetKernelArg", success))
                {
                    free_pointers(malloced_pointers, alligned_malloced_pointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg 4 - const int filterWidth

                success = clSetKernelArg(
                    blur_kernel,
                    4,
                    sizeof(filter_size),
                    &filter_size
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
                size_t global_work_offset[] = { 0, 0, 0 };
                size_t global_work_size[] = { lena_x, lena_y, 0 };
                size_t local_work_size[] = { 1, 1, 0 };

                // Get the maps ready for timing
                std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> lena_time_captures_enque;
                std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> lena_time_captures_finish;

                LARGE_INTEGER start_time, finish_time;

                const auto run_name_base = "Lena_" + std::to_string(lena_x) + "x" + std::to_string(lena_y) + "_filt_" + std::to_string(filter_size) + "_sig2_" + std::to_string(filter_sigma2);

                printf("Starting run timing for %d iterations on %s\n",kernel_iterations, run_name_base.c_str());
                for (uint32_t i = 0; i < kernel_iterations; ++i)
                {
                    // Do the captures for the enque time
                    QueryPerformanceCounter(&start_time);

                    success = clEnqueueNDRangeKernel(
                        chosen_command_queue,
                        blur_kernel,
                        2,
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

                    QueryPerformanceCounter(&finish_time);

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

                    lena_time_captures_enque.emplace_back(start_time, finish_time);

                    //Do the captures for the finish time
                    QueryPerformanceCounter(&start_time);

                    success = clEnqueueNDRangeKernel(
                        chosen_command_queue,
                        blur_kernel,
                        2,
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

                    //QueryPerformanceCounter(&finish_time);

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

                    QueryPerformanceCounter(&finish_time);

                    lena_time_captures_finish.emplace_back(start_time, finish_time);
                }
                printf("finished timing.\nPrinting results\n\n");

                const auto run_name_enque = run_name_base + "_enque_times";
                const auto run_name_finish = run_name_base + "_finish_times";
                const auto run_name_results = run_name_base + "_results.txt";

                const auto lena_enque_results = print_results(lena_time_captures_enque, run_name_enque);
                const auto lena_finish_results = print_results(lena_time_captures_finish, run_name_finish);

                results_outputs[run_name_enque] = lena_enque_results;
                results_outputs[run_name_finish] = lena_finish_results;
            }
        }
    }

    std::ofstream myfile;

    myfile.open(output_directory + "results.txt", std::ios::out);

    for (const auto &run : results_outputs)
    {

        myfile << run.first << ": Average = " << run.second.first << "ms; Standard Deviation = " <<
            run.second.second << std::endl;
    }

    myfile.close();

    free_pointers(malloced_pointers, alligned_malloced_pointers);
    return 0;
}
