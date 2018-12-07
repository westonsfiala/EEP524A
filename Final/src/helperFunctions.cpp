#include "helperFunctions.h"
#include "CL/cl.h"
#include <assert.h>
#include <Windows.h>
#include <iostream>
#include <fstream>
#include <sstream>

namespace Helper
{
    // Got this from
    // https://stackoverflow.com/questions/116038/what-is-the-best-way-to-read-an-entire-file-into-a-stdstring-in-c
    std::string slurp(const std::string& fileName) 
    {
        std::ifstream ifs(fileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);

        const auto fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::vector<char> bytes(fileSize);
        ifs.read(&bytes[0], fileSize);

        return std::string(&bytes[0], fileSize);
    }

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

    std::pair<double, double> printResults(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times, const std::string& filename)
    {
        std::ofstream myfile;

        const auto filePath = filename + ".csv";

        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);

        std::vector<double> samples;

        myfile.open(filePath, std::ios::out);

        for (const auto& timeStamp : times)
        {
            double elapsedTime = timeStamp.second.QuadPart - timeStamp.first.QuadPart;

            // Convert from number of counts to number of milliseconds
            elapsedTime *= 1000;
            elapsedTime /= frequency.QuadPart;

            samples.push_back(static_cast<double>(elapsedTime));

            myfile << elapsedTime << std::endl;
        }

        myfile.close();

        return { average(samples), standardDeviation(samples) };
    }

    std::pair<double, double> printResults(std::vector<std::pair<cl_ulong, cl_ulong>>& times, const std::string& filename)
    {
        std::ofstream myfile;

        const auto filePath = filename + ".csv";

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
        return { average(samples), standardDeviation(samples) };
    }

    uint8_t* convert1To4Channel(uint8_t* oneChannelData, const uint32_t& numPixels)
    {
        // ReSharper disable once CppLocalVariableMayBeConst
        auto fourChannelData = static_cast<uint8_t*>(malloc(sizeof(uint8_t) * numPixels * 4));

        const auto numChannelData = numPixels;
        for (uint32_t i = 0; i < numChannelData; i++)
        {
            const auto fourChannelIndex = i * 4;
            const auto rValue = oneChannelData[i];
            fourChannelData[fourChannelIndex] = rValue;
            fourChannelData[fourChannelIndex + 1] = rValue;
            fourChannelData[fourChannelIndex + 2] = rValue;
            fourChannelData[fourChannelIndex + 3] = static_cast<uint8_t>(255);
        }

        free(oneChannelData);

        return fourChannelData;
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


    /*
    * Given a cl code and return a string represenation
    * https://stackoverflow.com/questions/24326432/convenient-way-to-show-opencl-error-codes
    */
    std::string clGetErrorString(const int &errorCode)
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

    bool processClCallStatus(const std::string& callName, const int &errorCode)
    {
        // Success Case
        if (errorCode == CL_SUCCESS)
        {
#ifndef DONT_PRINT_CALL_SUCCESSES
            std::cout << callName << "(...) succeeded." << std::endl;
#endif
            return true;
        }
        // Everything else is a failure.

        std::cout << callName << "(...) failed with code:" << clGetErrorString(errorCode) << std::endl;
        return false;
    }

std::vector<Helper::MandelbrotSaveState> generateZeroState(const float left, const float top, const float xSide, const float ySide, const uint32_t xMax, const uint32_t yMax)
{
    std::vector<Helper::MandelbrotSaveState> saveStates;

    // setting up the xscale and yscale 
    const auto xScale = xSide / xMax;
    const auto yScale = ySide / yMax;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < yMax; y++)
    {
        for (auto x = 0; x < xMax; x++)
        {
            Helper::MandelbrotSaveState saveState;
            saveState.constantComplex = { x * xScale + left, y * yScale + top };
            saveState.complex = saveState.constantComplex;
            saveState.count = 1;
            saveState.adjustedCount = 1;

            saveStates.push_back(saveState);
        }
    }

    return saveStates;
}
}
