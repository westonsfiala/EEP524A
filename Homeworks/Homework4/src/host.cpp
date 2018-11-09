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
const static std::string PLATFORM_NAME_TO_USE = "Intel(R) OpenCL";
const static std::string PREFERED_DEVICE_NAME = "Intel(R) UHD Graphics 620";

/*************************************************************************
 * CHANGE THE FOLLOWING LINES TO WHERE EVER YOU HAVE EVERYTHING MAPPED TO.
 *************************************************************************/
//Set the output directories that need to be used.
static const std::string BASE_DIRECTORY = "C:/work/GitHub/EEP524A/Homeworks/Homework4/";
static const std::string OUTPUT_DIRECTORY = BASE_DIRECTORY + "Outputs/";
static const std::string SRC_DIRECTORY = BASE_DIRECTORY + "src/";
static const std::string BLUR_KERNEL_FILE = SRC_DIRECTORY + "Blur_Kernel.cl";
static const std::string LENA_FILE = SRC_DIRECTORY + "tiny-Lena_32bit.png";
static const std::string LITTLE_LENA_FILE = SRC_DIRECTORY + "little-Lena_24bit.png";
static const std::string NORMAL_LENA_FILE = SRC_DIRECTORY + "lena_512x512_32bit.png";
static const std::string BIG_LENA_FILE = SRC_DIRECTORY + "Big_Gray-Lena_8bit.png";

static const std::vector<std::string> LENA_FILES = {LENA_FILE, LITTLE_LENA_FILE, NORMAL_LENA_FILE, BIG_LENA_FILE};

const static std::string CONV_FILTER_KERNEL_NAME = "img_conv_filter";

const static std::vector<uint32_t> FILTER_SIZES = {5, 7, 9};
const static std::vector<float> FILTER_SIGMA2_S = {0.75f, 1.2f};

const static uint32_t KERNEL_ITERATIONS = 1;

// Sometimes the kernel prints out bad results. In this case just re run the entire kernel until the printed file size breaks this size.
const static uint32_t BAD_RESULTS_IMAGE_SIZE = 25000;

// Got from https://stackoverflow.com/questions/33268513/calculating-standard-deviation-variance-in-c
double variance(const std::vector<double>& samples)
{
    const auto size = samples.size();

    double variance = 0;
    auto t = samples[0];
    for (uint32_t i = 1; i < size; i++)
    {
        t += samples[i];
        const auto diff = (i + 1) * samples[i] - t;
        variance += diff * diff / ((i + 1.0) * i);
    }

    return variance / (size - 1);
}

double standardDeviation(const std::vector<double>& samples)
{
    return sqrt(variance(samples));
}

double average(const std::vector<double>& samples)
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

// https://stackoverflow.com/questions/5840148/how-can-i-get-a-files-size-in-c
long getFileSize(std::string filename)
{
    struct stat statBuf;
    const auto rc = stat(filename.c_str(), &statBuf);
    return rc == 0 ? statBuf.st_size : -1;
}

std::pair<double, double> printResults(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times, const std::string& filename)
{
    std::ofstream myfile;

    const auto filePath = OUTPUT_DIRECTORY + filename + ".csv";

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    std::vector<double> samples;

    myfile.open(filePath, std::ios::out);

    for (const auto& timeStamp : times)
    {
        double elapsedTime = timeStamp.second.QuadPart - timeStamp.first.QuadPart;

        // Convert from number of counts to number of milliseconds
        elapsedTime *= 1000000;
        elapsedTime /= frequency.QuadPart;

        samples.push_back(static_cast<double>(elapsedTime));

        myfile << elapsedTime << std::endl;
    }

    myfile.close();

    return {average(samples), standardDeviation(samples)};
}

std::pair<double, double> printResults(std::vector<std::pair<cl_ulong, cl_ulong>>& times, const std::string& filename)
{
    std::ofstream myfile;

    const auto filePath = OUTPUT_DIRECTORY + filename + ".csv";

    std::vector<double> samples;

    myfile.open(filePath, std::ios::out);

    for (const auto& timeStamp : times)
    {
        double elapsedTime = timeStamp.second - timeStamp.first;

        elapsedTime /= 1000000.0;

        samples.push_back(elapsedTime);

        myfile << elapsedTime << std::endl;
    }

    myfile.close();
    return {average(samples), standardDeviation(samples)};
}

uint8_t* convert3To4Channel(uint8_t* threeChannelData, const uint32_t& numPixels)
{
    // ReSharper disable once CppLocalVariableMayBeConst
    auto fourChannelData = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * numPixels * 4));

    const auto numChannelData = numPixels * 3;
    for (uint32_t i = 0; i < numChannelData; i += 3)
    {
        const auto fourChannelIndex = i * 4 / 3;
        assert(i * 4 % 3 == 0);
        fourChannelData[fourChannelIndex] = threeChannelData[i];
        fourChannelData[fourChannelIndex + 1] = threeChannelData[i + 1];
        fourChannelData[fourChannelIndex + 2] = threeChannelData[i + 2];
        fourChannelData[fourChannelIndex + 3] = static_cast<uint8_t>(255);
    }

    free(threeChannelData);

    return fourChannelData;
}

/**
 * \brief Frees up all of the pointers contained in the vectors.
 * \param unalignedPointers Vector to all the pointers that were generated with malloc.
 * \param alignedPointers Vector to all the pointers that were generated with alligned_malloc.
 */
void freePointers(std::vector<void*>& unalignedPointers, std::vector<void*>& alignedPointers)
{
    for (auto unalignedPointer : unalignedPointers)
    {
        free(unalignedPointer);
    }

    for (auto alignedPointer : alignedPointers)
    {
        _aligned_free(alignedPointer);
    }

    unalignedPointers.clear();
    alignedPointers.clear();
}

/*
* Given a cl code and return a string represenation
* https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
*/
const char* clGetErrorString(const int errorCode)
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

bool processClCallStatus(const std::string& callName, const cl_int errorCode)
{
    // Success Case
    if (errorCode == CL_SUCCESS)
    {
#ifndef DONT_PRINT_CALL_SUCCESSES
        printf("%s(...) succeeded.\n", call_name.c_str());
#endif
        return true;
    }
    // Everything else is a failure.

    printf("%s(...) failed with code: %s\n", callName.c_str(), clGetErrorString(errorCode));
    return false;
}

int main(int argc, char** argv)
{
    // Create containers to track all the pointers than need to be freed when we finish up.
    std::vector<void*> mallocedPointers;
    std::vector<void*> allignedMallocedPointers;

    cl_uint numPlatforms = 0;

    // Get the number of platforms
    auto success = clGetPlatformIDs(
        0,
        nullptr,
        &numPlatforms
    );

    if (!processClCallStatus("clGetPlatformIDs", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    const auto platforms = static_cast<cl_platform_id*>(malloc(sizeof(cl_platform_id) * numPlatforms));

    mallocedPointers.push_back(platforms);

    // Get the platforms
    success = clGetPlatformIDs(
        numPlatforms,
        platforms,
        nullptr
    );
    if (!processClCallStatus("clGetPlatformIDs", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    cl_platform_id chosenPlatform = nullptr;
    cl_device_id* chosenDevices = nullptr;
    cl_uint numChosenDevices = -1;

    for (cl_uint platformIndex = 0; platformIndex < numPlatforms; platformIndex++)
    {
        size_t paramValueSize;

        // Get the size of the name of the platform.
        success = clGetPlatformInfo(
            platforms[platformIndex],
            CL_PLATFORM_NAME,
            0,
            nullptr,
            &paramValueSize
        );
        if (!processClCallStatus("clGetPlatformInfo", success))
        {
            freePointers(mallocedPointers, allignedMallocedPointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        const auto platformName = static_cast<char*>(malloc(sizeof(char) * paramValueSize));
        mallocedPointers.push_back(platformName);

        success = clGetPlatformInfo(
            platforms[platformIndex],
            CL_PLATFORM_NAME,
            paramValueSize,
            platformName,
            nullptr
        );
        if (!processClCallStatus("clGetPlatformInfo", success))
        {
            freePointers(mallocedPointers, allignedMallocedPointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        auto platName = std::string(platformName) + std::string("\n");
        printf("Platform: %s\n", platName.c_str());

        if (platformName != PLATFORM_NAME_TO_USE)
        {
            printf("Skipping unwanted platform.\n\n");
            continue;
        }

        cl_uint numDevices;

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platformIndex],
            CL_DEVICE_TYPE_GPU,
            0,
            nullptr,
            &numDevices
        );
        // Their is an issue where the CPU only experimental build will fail, just skip that one.
        if (success == -1)
        {
            continue;
        }
        if (!processClCallStatus("clGetDeviceIDs", success))
        {
            freePointers(mallocedPointers, allignedMallocedPointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        auto devices = static_cast<cl_device_id*>(malloc(sizeof(cl_device_id) * numDevices));
        mallocedPointers.push_back(devices);

        // Get the number of devices
        success = clGetDeviceIDs(
            platforms[platformIndex],
            CL_DEVICE_TYPE_GPU,
            numDevices,
            devices,
            &numDevices
        );
        if (!processClCallStatus("clGetDeviceIDs", success))
        {
            freePointers(mallocedPointers, allignedMallocedPointers);
            // In debug its nice to hit an assert so that we stop and 
            // can see what are the things that were sent in to break it.
            assert(false);
            return -1;
        }

        auto deviceTaken = false;

        for (cl_uint deviceIndex = 0; deviceIndex < numDevices; deviceIndex++)
        {
            // Get info from the devices

            // Get the device name
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_NAME,
                0,
                nullptr,
                &paramValueSize
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            const auto deviceName = static_cast<char*>(malloc(paramValueSize));
            mallocedPointers.push_back(deviceName);
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_NAME,
                paramValueSize,
                deviceName,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            const std::string deviceNameString = deviceName;
            auto devName = deviceNameString + std::string("\n");
            printf("%s", devName.c_str());

            if (deviceNameString == PREFERED_DEVICE_NAME)
            {
                deviceTaken = true;
            }

            // Get the double config
            cl_device_fp_config fpConfig;
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_DOUBLE_FP_CONFIG,
                sizeof(cl_device_fp_config),
                &fpConfig,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Double FP Config : %X\n", static_cast<int>(fpConfig));

            // Get the preferred vector width float
            cl_uint preferedVectorWidthFloat;
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT,
                sizeof(cl_uint),
                &preferedVectorWidthFloat,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Prefered Vector Width Float : %u\n", preferedVectorWidthFloat);

            // Get the preferred vector width float
            cl_uint nativeVectorWidthFloat;
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT,
                sizeof(cl_uint),
                &nativeVectorWidthFloat,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Native Vector Width Float : %u\n", nativeVectorWidthFloat);

            // Get the max clock frequency
            cl_uint maxClockFrequency;
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_MAX_CLOCK_FREQUENCY,
                sizeof(cl_uint),
                &maxClockFrequency,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Clock Frequency : %u\n", maxClockFrequency);

            // Get the max compute units
            cl_uint maxComputeUnits;
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_MAX_COMPUTE_UNITS,
                sizeof(cl_uint),
                &maxComputeUnits,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Compute Units : %u\n", maxComputeUnits);

            // Get the max work item sizes
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                0,
                nullptr,
                &paramValueSize
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            size_t* maxWorkItemSizes = static_cast<size_t*>(malloc(paramValueSize));
            mallocedPointers.push_back(maxWorkItemSizes);
            success = clGetDeviceInfo(
                devices[deviceIndex],
                CL_DEVICE_MAX_WORK_ITEM_SIZES,
                paramValueSize,
                maxWorkItemSizes,
                nullptr
            );
            if (!processClCallStatus("clGetDeviceInfo", success))
            {
                freePointers(mallocedPointers, allignedMallocedPointers);
                // In debug its nice to hit an assert so that we stop and 
                // can see what are the things that were sent in to break it.
                assert(false);
                return -1;
            }
            printf("Max Work Item Sizes:\n");
            printf("\tMax Work Item Size x : %llu\n", static_cast<uint64_t>(maxWorkItemSizes[0]));
            printf("\tMax Work Item Size y : %llu\n", static_cast<uint64_t>(maxWorkItemSizes[1]));
            printf("\tMax Work Item Size z : %llu\n", static_cast<uint64_t>(maxWorkItemSizes[2]));

            // clean up the output.
            printf("\n");

            // Select the platform & device that we are using by assigning it here.
            chosenPlatform = platforms[platformIndex];
            chosenDevices = devices;
            numChosenDevices = numDevices;
            // If we have our prefered device, don't keep searching.
            if (deviceTaken)
            {
                printf("Device Found. Stopping Search.\n");
                break;
            }
        }
    }

    cl_context_properties properties[3] = {
        CL_CONTEXT_PLATFORM, reinterpret_cast<cl_context_properties>(chosenPlatform), 0
    };

    // Create the context for our run.
    const auto chosenContext = clCreateContext(
        properties,
        numChosenDevices,
        chosenDevices,
        nullptr,
        nullptr,
        &success
    );
    if (!processClCallStatus("clCreateContext", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    size_t blurKernelCharSize;

    const auto blurKernelString = read_source(BLUR_KERNEL_FILE.c_str(), &blurKernelCharSize);

    std::string checkString1 = blurKernelString;

    assert(blurKernelString);

    // The strings array is filled with the two null terminated strings.
    const char* strings[] = {blurKernelString};
    // Because the strings are null terminated, we pass in 0.
    const size_t lengths[] = {0};

    mallocedPointers.push_back(blurKernelString);

    // Create the program
    const auto chosenProgram = clCreateProgramWithSource(
        chosenContext,
        1,
        strings,
        lengths,
        &success
    );
    if (!processClCallStatus("clCreateProgramWithSource", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Build the program
    success = clBuildProgram(
        chosenProgram,
        numChosenDevices,
        chosenDevices,
        "-cl-std=CL2.0",
        nullptr,
        nullptr
    );
    if (!processClCallStatus("clBuildProgram", success))
    {
        // Get detailed build errors when they occur.
        size_t buildErrorLogSize;
        clGetProgramBuildInfo(chosenProgram, chosenDevices[0], CL_PROGRAM_BUILD_LOG, 0, nullptr,
            &buildErrorLogSize);

        auto buildError = static_cast<char*>(malloc(sizeof(char) * buildErrorLogSize));
        mallocedPointers.push_back(buildError);

        clGetProgramBuildInfo(chosenProgram, chosenDevices[0], CL_PROGRAM_BUILD_LOG, buildErrorLogSize,
            buildError, nullptr);

        std::string errorString = buildError;
        printf("%s\n", errorString.c_str());

        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    //cl_queue_properties queue_properties[3] = {CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0};

    // Create the command queue
    const auto chosenCommandQueue = clCreateCommandQueueWithProperties(
        chosenContext,
        chosenDevices[0],
        nullptr, //queue_properties,
        &success
    );
    if (!processClCallStatus("clCreateCommandQueueWithProperties", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    // Create the naive Kernel
    const auto blurKernel = clCreateKernel(
        chosenProgram,
        CONV_FILTER_KERNEL_NAME.c_str(),
        &success
    );
    if (!processClCallStatus("clCreateKernel", success))
    {
        freePointers(mallocedPointers, allignedMallocedPointers);
        // In debug its nice to hit an assert so that we stop and 
        // can see what are the things that were sent in to break it.
        assert(false);
        return -1;
    }

    std::map<std::string, std::pair<double, double>> resultsOutputs;

    for (const auto& lenaFile : LENA_FILES)
    {
        for (const auto& filterSize : FILTER_SIZES)
        {
            for (auto filterQueue = 0; filterQueue < FILTER_SIGMA2_S.size(); ++filterQueue)
            {
                auto filterSigma2 = FILTER_SIGMA2_S[filterQueue];
                const cl_mem_flags lenaInputImageFlags = CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY;
                const cl_mem_flags lenaOutputImageFlags = CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY;

                // Assign variables
                // Blur Kernel

                // Arg 0 - __read_only image2d_t inputImg
                int32_t lenaX;
                int32_t lenaY;
                int32_t lenaNumChannels;
                auto lenaData = static_cast<uint8_t*>(stbi_load(lenaFile.c_str(), &lenaX, &lenaY, &lenaNumChannels, 0));
                assert(lenaData);

                cl_image_format lenaImageFormat;
                lenaImageFormat.image_channel_data_type = CL_UNSIGNED_INT8;
                switch (lenaNumChannels)
                {
                case 1:
                    lenaImageFormat.image_channel_order = CL_R;
                    break;
                case 2:
                    lenaImageFormat.image_channel_order = CL_RA;
                    break;
                case 3:
                    // My board does not support 3 channels of data, so I just add a 4th A channel with its value set to 255.
                    lenaData = convert3To4Channel(lenaData, lenaX * lenaY);
                    lenaNumChannels = 4;
                case 4:
                    lenaImageFormat.image_channel_order = CL_RGBA;
                    break;
                default:
                    assert(false);
                    break;
                }

                // lena_data pointer may be swapped out in the case of 3 channels of data.
                mallocedPointers.push_back(lenaData);

                cl_image_desc lenaImageDesc;
                lenaImageDesc.image_type = CL_MEM_OBJECT_IMAGE2D;
                lenaImageDesc.image_width = lenaX;
                lenaImageDesc.image_height = lenaY;
                lenaImageDesc.image_depth = 0;
                lenaImageDesc.image_array_size = 0;
                lenaImageDesc.image_row_pitch = 0;
                lenaImageDesc.image_slice_pitch = 0;
                lenaImageDesc.num_mip_levels = 0;
                lenaImageDesc.num_samples = 0;
                lenaImageDesc.mem_object = nullptr;

                auto lenaImageMem = clCreateImage(
                    chosenContext,
                    lenaInputImageFlags,
                    &lenaImageFormat,
                    &lenaImageDesc,
                    lenaData,
                    &success
                );
                if (!processClCallStatus("clCreateImage", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blurKernel,
                    0,
                    sizeof lenaImageMem,
                    &lenaImageMem
                );
                if (!processClCallStatus("clSetKernelArg", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg1 - __write_only image2d_t outputImg,
                const auto lenaOutputData = malloc(sizeof(char) * lenaX * lenaY * lenaNumChannels);
                mallocedPointers.push_back(lenaOutputData);
                assert(lenaData);

                auto lenaOutputImageMem = clCreateImage(
                    chosenContext,
                    lenaOutputImageFlags,
                    &lenaImageFormat,
                    &lenaImageDesc,
                    lenaData,
                    &success
                );
                if (!processClCallStatus("clCreateImage", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blurKernel,
                    1,
                    sizeof lenaOutputImageMem,
                    &lenaOutputImageMem
                );
                if (!processClCallStatus("clSetKernelArg", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg2 - sampler_t sampler

                cl_sampler_properties lenaSamplerProperties[] = {
                    CL_SAMPLER_NORMALIZED_COORDS, false,
                    CL_SAMPLER_ADDRESSING_MODE, CL_ADDRESS_CLAMP_TO_EDGE,
                    CL_SAMPLER_FILTER_MODE, CL_FILTER_NEAREST, 0
                };

                auto lenaSampler = clCreateSamplerWithProperties(
                    chosenContext,
                    lenaSamplerProperties,
                    &success
                );

                if (!processClCallStatus("clCreateSamplerWithProperties", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blurKernel,
                    2,
                    sizeof lenaSampler,
                    &lenaSampler
                );
                if (!processClCallStatus("clSetKernelArg", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg 3 - __global float* filter

                // Get a vector of all the filter coefs.
                auto blurFilter = generate_gauss_blur(filterSize, filterSigma2);

                auto blurFilterMem = clCreateBuffer(
                    chosenContext,
                    CL_MEM_USE_HOST_PTR,
                    sizeof(float) * blurFilter.size(),
                    blurFilter.data(),
                    &success
                );
                if (!processClCallStatus("clCreateBuffer", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                success = clSetKernelArg(
                    blurKernel,
                    3,
                    sizeof blurFilterMem,
                    &blurFilterMem
                );
                if (!processClCallStatus("clSetKernelArg", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Arg 4 - const int filterWidth

                success = clSetKernelArg(
                    blurKernel,
                    4,
                    sizeof filterSize,
                    &filterSize
                );
                if (!processClCallStatus("clSetKernelArg", success))
                {
                    freePointers(mallocedPointers, allignedMallocedPointers);
                    // In debug its nice to hit an assert so that we stop and 
                    // can see what are the things that were sent in to break it.
                    assert(false);
                    return -1;
                }

                // Enque the kernel
                size_t globalWorkOffset[] = {0, 0, 0};
                size_t globalWorkSize[] = {lenaX, lenaY, 0};
                size_t localWorkSize[] = {1, 1, 0};

                // Get the maps ready for timing
                std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> lenaTimeCapturesEnque;
                std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>> lenaTimeCapturesFinish;

                LARGE_INTEGER startTime, finishTime;

                const auto runNameBase = "Lena_" + std::to_string(lenaX) + "x" + std::to_string(lenaY) + "_filt_" + std::to_string(filterSize) + "_sig2_" + std::to_string(filterSigma2);

                printf("Starting run timing for %d iterations on %s\n", KERNEL_ITERATIONS, runNameBase.c_str());

                auto imagePrinted = false;

                for (uint32_t i = 0; i < KERNEL_ITERATIONS; ++i)
                {
                    // Do the captures for the enque time
                    QueryPerformanceCounter(&startTime);

                    success = clEnqueueNDRangeKernel(
                        chosenCommandQueue,
                        blurKernel,
                        2,
                        globalWorkOffset,
                        globalWorkSize,
                        localWorkSize,
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

                    QueryPerformanceCounter(&finishTime);

                    // Wait for the kernel to finish
                    success = clFinish(chosenCommandQueue);
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

                    // Sometimes we have to keep running the kernel to get a good picture.
                    lenaTimeCapturesEnque.emplace_back(startTime, finishTime);

                    //Do the captures for the finish time
                    QueryPerformanceCounter(&startTime);

                    success = clEnqueueNDRangeKernel(
                        chosenCommandQueue,
                        blurKernel,
                        2,
                        globalWorkOffset,
                        globalWorkSize,
                        localWorkSize,
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
                    success = clFinish(chosenCommandQueue);
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

                    QueryPerformanceCounter(&finishTime);


                    // Sometimes we have to keep running the kernel to get a good picture.

                    lenaTimeCapturesFinish.emplace_back(startTime, finishTime);

                    if (!imagePrinted)
                    {
                        // Get the image back out of the device.
                        size_t origin[] = { 0, 0, 0 };
                        size_t region[] = { lenaX, lenaY, 1 };
                        size_t rowPitch = 0;
                        size_t slicePitch = 0;

                        const auto lenaOutputImageData = clEnqueueMapImage(
                            chosenCommandQueue,
                            lenaOutputImageMem,
                            true,
                            CL_MAP_READ,
                            origin,
                            region,
                            &rowPitch,
                            &slicePitch,
                            0,
                            nullptr,
                            nullptr,
                            &success
                        );
                        if (!processClCallStatus("clEnqueueMapImage", success))
                        {
                            freePointers(mallocedPointers, allignedMallocedPointers);
                            // In debug its nice to hit an assert so that we stop and
                            // can see what are the things that were sent in to break it.
                            assert(false);
                            return -1;
                        }

                        const auto runNamePrint = OUTPUT_DIRECTORY + runNameBase + "_image.png";

                        success = stbi_write_png(runNamePrint.c_str(), lenaX, lenaY, lenaNumChannels, lenaOutputImageData, sizeof(char) * lenaX * lenaNumChannels);
                        assert(success);

                        success = clEnqueueUnmapMemObject(
                            chosenCommandQueue,
                            lenaOutputImageMem,
                            lenaOutputImageData,
                            0,
                            nullptr,
                            nullptr
                        );
                        if (!processClCallStatus("clEnqueueUnmapMemObject", success))
                        {
                            freePointers(mallocedPointers, allignedMallocedPointers);
                            // In debug its nice to hit an assert so that we stop and
                            // can see what are the things that were sent in to break it.
                            assert(false);
                            return -1;
                        }

                        const auto imageSize = getFileSize(runNamePrint);

                        // If the image size is too small, we will have a trash image, remake all the data and try to run it again.
                        if (imageSize < BAD_RESULTS_IMAGE_SIZE)
                        {
                            filterQueue--;
                            printf("Image Creation failed. Restarting filter run.\n");
                            break;
                        }

                        imagePrinted = true;
                    }
                }


                printf("finished timing.\nPrinting results\n\n");

                const auto runNameEnque = runNameBase + "_enque_times";
                const auto runNameFinish = runNameBase + "_finish_times";
                const auto runNameResults = runNameBase + "_results.txt";

                const auto lenaEnqueResults = printResults(lenaTimeCapturesEnque, runNameEnque);
                const auto lenaFinishResults = printResults(lenaTimeCapturesFinish, runNameFinish);

                resultsOutputs[runNameEnque] = lenaEnqueResults;
                resultsOutputs[runNameFinish] = lenaFinishResults;

                
            }
        }
    }

    std::ofstream myfile;

    myfile.open(OUTPUT_DIRECTORY + "results.txt", std::ios::out);

    for (const auto& run : resultsOutputs)
    {
        myfile << run.first << ": Average = " << run.second.first << "ms; Standard Deviation = " <<
            run.second.second << std::endl;
    }

    myfile.close();

    freePointers(mallocedPointers, allignedMallocedPointers);
    return 0;
}
