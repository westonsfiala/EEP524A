
// Include the c++ wrapper of openCL and enable exceptions
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>
#include <iostream>

#include "helperFunctions.h"

/*************************************************************************
* CHANGE THE FOLLOWING LINES TO WHERE EVER YOU HAVE EVERYTHING MAPPED TO.
*************************************************************************/
//Set the output directories that need to be used.
static const std::string BASE_DIRECTORY = "C:/work/GitHub/EEP524A/Final/";

static const std::string OUTPUT_DIRECTORY = BASE_DIRECTORY + "Outputs/";
static const std::string SRC_DIRECTORY = BASE_DIRECTORY + "src/";
static const std::string OUTPUT_GLOBAL_KERNEL_NAME = "outputGlobal";
static const std::string OUTPUT_GLOBAL_KERNEL_FILE = SRC_DIRECTORY + OUTPUT_GLOBAL_KERNEL_NAME + ".cl";

static const int X_MAX = 256;
static const int Y_MAX = 256;
static const int Z_MAX = 256;

int main()
{
    // Go through all the platforms and find a good one.
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    cl::Platform chosenPlatform;

    // Go until we find a valid platform.
    for (auto &plat : platforms) {
        const auto profile = plat.getInfo<CL_PLATFORM_PROFILE>();
        const auto version = plat.getInfo<CL_PLATFORM_VERSION>();
        const auto name = plat.getInfo<CL_PLATFORM_NAME>();
        const auto vendor = plat.getInfo<CL_PLATFORM_VENDOR>();
        const auto extensions = plat.getInfo<CL_PLATFORM_EXTENSIONS>();

        // Print out the information about the one we choose.
        if (version.find("OpenCL 2.") != std::string::npos) {

            std::cout << "Platform Info:" << std::endl;
            std::cout << "name: " << name << std::endl;
            std::cout << "version: " << version << std::endl;
            std::cout << "profile: " << profile << std::endl;
            std::cout << "vendor: " << vendor << std::endl;
            std::cout << "extensions: " << extensions << std::endl << std::endl;

            chosenPlatform = plat;
            break;
        }
    }

    // If no platform was found, give up.
    if (chosenPlatform() == nullptr)  {
        std::cout << "No OpenCL 2.0 platform found.";
        return -1;
    }

    // Set the platform we found as the default.
    const auto newDefaultPlatform = cl::Platform::setDefault(chosenPlatform);
    if (newDefaultPlatform != chosenPlatform) {
        std::cout << "Error setting default platform.";
        return -1;
    }

    // Go through the devices in the platform and print out their info.
    try
    {
        std::vector<cl::Device> devices;

        // ReSharper disable once CppExpressionWithoutSideEffects
        chosenPlatform.getDevices(CL_DEVICE_TYPE_GPU, &devices);

        for(auto device : devices)
        {
            std::cout << "Device Info:" << std::endl;
            const auto name = device.getInfo<CL_DEVICE_NAME>();
            std::cout << "name: " << name << std::endl;
            const auto vendor = device.getInfo<CL_DEVICE_VENDOR>();
            std::cout << "vendor: " << vendor << std::endl;
            const auto version = device.getInfo<CL_DEVICE_VERSION>();
            std::cout << "version: " << version << std::endl;
            const auto maxComputeUnits = device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
            std::cout << "maxComputeUnits: " << maxComputeUnits << std::endl;
            const auto maxWorkItemDimentions = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
            std::cout << "maxWorkItemDimentions: " << maxWorkItemDimentions << std::endl;
            const auto maxWorkGroupSize = device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>();
            std::cout << "maxWorkGroupSize: " << maxWorkGroupSize << std::endl;
            const auto maxWorkItemSize = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
            std::cout << "maxWorkItemSize:" << std::endl; 
            std::cout << "\tx: " << maxWorkItemSize[0] <<std::endl;
            std::cout << "\ty: " << maxWorkItemSize[1] << std::endl;
            std::cout << "\tz: " << maxWorkItemSize[2] << std::endl;
            const auto preferredVectorWidthFloat = device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
            std::cout << "preferredVectorWidthFloat: " << preferredVectorWidthFloat << std::endl;
            const auto preferredVectorWidthDouble = device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
            std::cout << "preferredVectorWidthDouble: " << preferredVectorWidthDouble << std::endl;
            const auto nativeVectorWidthFloat = device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>();
            std::cout << "nativeVectorWidthFloat: " << nativeVectorWidthFloat << std::endl;
            const auto nativeVectorWidthDouble = device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>();
            std::cout << "nativeVectorWidthDouble: " << nativeVectorWidthDouble << std::endl;
            std::cout << std::endl;
        }
    }
    catch(...)
    {
        std::cout << "Error getting devices" << std::endl;
        return -1;
    }

    // Get the kernel string for the output global ID kernel.
    const auto kernelString = Helper::slurp(OUTPUT_GLOBAL_KERNEL_FILE);
    const std::vector<std::string> programStrings {kernelString};


    cl::Program outputGlobalIdProgram(programStrings);
    try {
        // ReSharper disable once CppExpressionWithoutSideEffects
        outputGlobalIdProgram.build("-cl-std=CL2.0");
    }
    catch (...) {
        // Print build info for all devices
        auto buildErr = CL_SUCCESS;
        auto buildInfo = outputGlobalIdProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto &pair : buildInfo) {
            std::cerr << pair.second << std::endl << std::endl;
        }
        return -1;
    }

    // Setup the kernel so that when we have the arguments we can run it right away.
    auto runKernel = cl::KernelFunctor<
        cl::Buffer
        >(outputGlobalIdProgram, OUTPUT_GLOBAL_KERNEL_NAME);

    auto numElements = X_MAX * Y_MAX * Z_MAX;
    std::vector<int> outputData(numElements, 0xdeadbeef);
    cl::Buffer outputBuffer(outputData.begin(), outputData.end(), false);

    runKernel(
        cl::EnqueueArgs(cl::NDRange(X_MAX, Y_MAX, Z_MAX)),
        outputBuffer
        );


    cl::copy(outputBuffer, outputData.begin(), outputData.end());

    std::cout << "Output Data Validation : ";

    for(auto z = 0; z < Z_MAX; ++z)
    {
        for(auto y = 0; y < Y_MAX; ++y)
        {
            for(auto x = 0; x < X_MAX; ++x)
            {
                auto index = z * Y_MAX * X_MAX + y * X_MAX + x;
                auto value = z << 16 | y << 8 | x;
                if(outputData[index] != value)
                {
                    std::cout << "Failed" << std::endl;
                    std::cout << "Expected [" << outputData[index] << "] - Got [" << value << "]" << std::endl;
                    return -1;
                }
            }
        }
    }

    std::cout << "Succeeded" << std::endl;

    return 0;
}