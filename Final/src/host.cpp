// Include the c++ wrapper of openCL and enable exceptions
#include <iostream>


#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 200
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>

#include "helperFunctions.h"

#include <vector>
#include <ostream>
#include <iostream>
#include <SDL.h>
#include <chrono>
#include <map>
#include <numeric>
#include <assert.h>
#include <float.h>

/*************************************************************************
* CHANGE THE FOLLOWING LINES TO WHERE EVER YOU HAVE EVERYTHING MAPPED TO.
*************************************************************************/
//Set the output directories that need to be used.
static const std::string BASE_DIRECTORY = "C:/work/GitHub/EEP524A/Final/";

static const std::string OUTPUT_DIRECTORY = BASE_DIRECTORY + "Outputs/";
static const std::string SRC_DIRECTORY = BASE_DIRECTORY + "src/";
static const std::string MANDELBROT_KERNEL_NAME = "Mandelbrot";
static const std::string MANDELBROT_KERNEL_FILE = SRC_DIRECTORY + MANDELBROT_KERNEL_NAME + ".cl";

static const std::string OUTPUT_GLOBAL_KERNEL_NAME = "OutputGlobal";
static const std::string OUTPUT_GLOBAL_KERNEL_FILE = SRC_DIRECTORY + OUTPUT_GLOBAL_KERNEL_NAME + ".cl";

static const auto MAX_ITERATIONS = 100;

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0.0f, 0.0f);
static const auto MIN_ORDER = 1.0f;
static const auto ORDER = 15.0f;

static const float DEFAULT_FIT = 2.5f;

struct  MandelbrotSaveState
{
    cl_float2 complex;
    cl_float2 constantComplex;
    cl_uint count;
    cl_float adjustedCount;
    cl_float2 padding; // Needed to pack into the necessary svm alignment.
} ;

    std::vector<MandelbrotSaveState> generateZeroState(const float left, const float top, const float xSide, const float ySide, const uint32_t xMax, const uint32_t yMax)
    {
        std::vector<MandelbrotSaveState> saveStates;
        // setting up the xscale and yscale 
        const auto xScale = xSide / xMax;
        const auto yScale = ySide / yMax;

        // scanning every point in that rectangular area. 
        // Each point represents a Complex number (x + yi). 
        // Iterate that complex number 
        for (uint32_t y = 0; y < yMax; y++)
        {
            for (uint32_t x = 0; x < xMax; x++)
            {
                MandelbrotSaveState saveState;
                saveState.constantComplex = { x * xScale + left, y * yScale + top };
                saveState.complex = saveState.constantComplex;
                saveState.count = 1;
                saveState.adjustedCount = 0.0f;

                saveStates.push_back(saveState);
            }
        }

        return saveStates;
    }

int main(int argc, char** argv)
{
    // Go through all the platforms and find a good one.
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);

    cl::Platform chosenPlatform;

    auto is2 = false;

    // Go until we find a valid platform.
    for (auto &plat : platforms) {
        const auto profile = plat.getInfo<CL_PLATFORM_PROFILE>();
        const auto version = plat.getInfo<CL_PLATFORM_VERSION>();
        const auto name = plat.getInfo<CL_PLATFORM_NAME>();
        const auto vendor = plat.getInfo<CL_PLATFORM_VENDOR>();
        const auto extensions = plat.getInfo<CL_PLATFORM_EXTENSIONS>();

        // Print out the information about the one we choose.
        if (version.find("OpenCL 2.") != std::string::npos && 
            name.find("CPU Only") == std::string::npos && 
            name.find("Intel") != std::string::npos) 
        {
            std::cout << "Platform Info:" << std::endl;
            std::cout << "name: " << name << std::endl;
            std::cout << "version: " << version << std::endl;
            std::cout << "profile: " << profile << std::endl;
            std::cout << "vendor: " << vendor << std::endl;
            std::cout << "extensions: " << extensions << std::endl << std::endl;

            chosenPlatform = plat;
            is2 = true;
            break;
        }
    }

    // If we didn't find a 2.x one try for a 1.2
    if (chosenPlatform() == nullptr)
    {
        // Go until we find a valid platform.
        for (auto &plat : platforms) {
            const auto profile = plat.getInfo<CL_PLATFORM_PROFILE>();
            const auto version = plat.getInfo<CL_PLATFORM_VERSION>();
            const auto name = plat.getInfo<CL_PLATFORM_NAME>();
            const auto vendor = plat.getInfo<CL_PLATFORM_VENDOR>();
            const auto extensions = plat.getInfo<CL_PLATFORM_EXTENSIONS>();

            // Print out the information about the one we choose.
            if (version.find("OpenCL 1.") != std::string::npos && 
                name.find("Intel") != std::string::npos) 
            {
                std::cout << "Platform Info:" << std::endl;
                std::cout << "name: " << name << std::endl;
                std::cout << "version: " << version << std::endl;
                std::cout << "profile: " << profile << std::endl;
                std::cout << "vendor: " << vendor << std::endl;
                std::cout << "extensions: " << extensions << std::endl << std::endl;

                chosenPlatform = plat;
                is2 = false;
                break;
            }
        }
    }

    // If no platform was found, give up.
    if (chosenPlatform() == nullptr)  {
        std::cout << "No OpenCL 1.x or 2.x platform found.";
        return -1;
    }

    // Set the platform we found as the default.
    const auto newDefaultPlatform = cl::Platform::setDefault(chosenPlatform);
    if (newDefaultPlatform != chosenPlatform) {
        std::cout << "Error setting default platform.";
        return -1;
    }

    uint32_t maxWorkGroupSize = 0;
    std::vector<uint32_t> maxWorkItemSize(3,0);

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
            const auto maxWorkItemDimensions = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS>();
            std::cout << "maxWorkItemDimensions: " << maxWorkItemDimensions << std::endl;
            maxWorkGroupSize = static_cast<uint32_t>(device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>());
            std::cout << "maxWorkGroupSize: " << maxWorkGroupSize << std::endl;
            auto workItemSize = device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
            std::cout << "maxWorkItemSize:" << std::endl; 
            std::cout << "\tx: " << workItemSize[0] <<std::endl;
            std::cout << "\ty: " << workItemSize[1] << std::endl;
            std::cout << "\tz: " << workItemSize[2] << std::endl;
            // Save these for the optimizations later.
            maxWorkItemSize[0] = static_cast<uint32_t>(workItemSize[0]);
            maxWorkItemSize[1] = static_cast<uint32_t>(workItemSize[1]);
            maxWorkItemSize[2] = static_cast<uint32_t>(workItemSize[2]);
            const auto preferredVectorWidthFloat = device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT>();
            std::cout << "preferredVectorWidthFloat: " << preferredVectorWidthFloat << std::endl;
            const auto preferredVectorWidthDouble = device.getInfo<CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE>();
            std::cout << "preferredVectorWidthDouble: " << preferredVectorWidthDouble << std::endl;
            const auto nativeVectorWidthFloat = device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>();
            std::cout << "nativeVectorWidthFloat: " << nativeVectorWidthFloat << std::endl;
            const auto nativeVectorWidthDouble = device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>();
            std::cout << "nativeVectorWidthDouble: " << nativeVectorWidthDouble << std::endl;
            const auto svmCapabilities = device.getInfo<CL_DEVICE_SVM_CAPABILITIES>();
            std::cout << "svmCapabilities: " << std::endl;
            if (svmCapabilities & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER)
            {
                std::cout << "\tCoarse Grain Buffer" << std::endl;
            }
            if (svmCapabilities & CL_DEVICE_SVM_FINE_GRAIN_BUFFER)
            {
                std::cout << "\tFine Grain Buffer" << std::endl;
            }
            if (svmCapabilities & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM)
            {
                std::cout << "\tFine Grain System" << std::endl;
            }
            if (svmCapabilities & CL_DEVICE_SVM_ATOMICS)
            {
                std::cout << "\tAtomics" << std::endl;
            }
            std::cout << std::endl;
        }
    }
    catch(...)
    {
        std::cout << "Error getting devices" << std::endl;
        return -1;
    }

    /*
    const auto outputKernelString = Helper::slurp(OUTPUT_GLOBAL_KERNEL_FILE);
    const std::vector<std::string> outputProgramStrings{ outputKernelString };

    cl::Program outputGlobalProgram(outputProgramStrings);
    try {
        std::string buildString;
        if (is2)
        {
            buildString = "-cl-std=CL2.0";
        }
        else
        {
            buildString = "-cl-std=CL1.2";
        }
        // ReSharper disable CppExpressionWithoutSideEffects
        outputGlobalProgram.build(buildString.c_str());
        // ReSharper restore CppExpressionWithoutSideEffects
    }
    catch (...) {
        // Print build info for all devices
        auto buildErr = CL_SUCCESS;
        auto buildInfo = outputGlobalProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto &pair : buildInfo) {
            std::cerr << pair.second << std::endl << std::endl;
        }
        return -1;
    }

    auto outputRunKernel = cl::KernelFunctor<
        cl::fine_svm_vector<cl_float2>&,
        cl::fine_svm_vector<cl_float2>&
    >(outputGlobalProgram, OUTPUT_GLOBAL_KERNEL_NAME);

    cl_float2 ones = { 1.0f, 1.0f };
    cl_float2 negativeOnes = { -1.0f, -1.0f };

    cl::fine_svm_vector<cl_float2> inputVector(10, ones);
    cl::fine_svm_vector<cl_float2> outputVector(10, negativeOnes);

    outputRunKernel(
        cl::EnqueueArgs(cl::NDRange(10)),
        inputVector,
        outputVector
    );
    */

    // Get the kernel string for the mandelbrot kernel.
    const auto kernelString = Helper::slurp(MANDELBROT_KERNEL_FILE);
    const std::vector<std::string> programStrings {kernelString};

    cl::Program mandelbrotProgram(programStrings);
    try {

        std::string buildString;
        if(is2)
        {
            buildString = "-cl-std=CL2.0";
        }
        else
        {
            buildString = "-cl-std=CL1.2";
        }
        // ReSharper disable once CppExpressionWithoutSideEffects
        mandelbrotProgram.build(buildString.c_str());
        // ReSharper restore CppExpressionWithoutSideEffects
    }
    catch (...) {
        // Print build info for all devices
        auto buildErr = CL_SUCCESS;
        auto buildInfo = mandelbrotProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto &pair : buildInfo) {
            std::cerr << pair.second << std::endl << std::endl;
        }
        return -1;
    }

    // Initialize the SDL library
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
    {
        std::cout << "SDL_GetCurrentDisplayMode Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    const auto width = dm.w;
    const auto height = dm.h;

    // Get the Screen Ratio
    const auto screenRatio = static_cast<float>(width) / height;

    // Center it and set the zoom level.
    // I don't know why this number is needed, but it centers it.
    const auto left = (-DEFAULT_FIT + 0.28125f) / ZOOM - CENTER.first;
    const auto top = -DEFAULT_FIT / 2 / ZOOM + CENTER.second;
    const auto xSide = 2.5f * screenRatio / ZOOM;
    const auto ySide = 2.5f / ZOOM;

    // Setup the kernel so that when we have the arguments we can run it right away.
    auto runKernel = cl::KernelFunctor<
        cl::Buffer, // FractalState
        cl::Buffer, // OutputPixels
        cl::Buffer, // Colors
        uint32_t, // numColors
        uint32_t, // maxCount
        cl_float, // order
        cl_float // bailout
        >(mandelbrotProgram, MANDELBROT_KERNEL_NAME);

    std::vector<cl_char> outputPixels(height * width * 3, 0);

    const cl::Buffer outputPixelBuffer(outputPixels.begin(), outputPixels.end(), false);

    // Set the global colors so that it is a fast lookup.
    auto setColors = Helper::setGlobalColorsFade({});

    const auto numColors = static_cast<uint32_t>(setColors.size() / 3);

    const cl::Buffer colorBuffer(setColors.begin(), setColors.end(), true);

    std::map<std::pair<uint32_t, uint32_t>, double> timingMap;

    std::cout << "Starting local size optimization." << std::endl;

    // Save the zero state so that we don't have to regenerate it all the time.
    auto zeroState = generateZeroState(left, top, xSide, ySide, width, height);
    std::vector<MandelbrotSaveState> checkState(zeroState.begin(), zeroState.end());

    for (uint32_t localX = 1; localX <= maxWorkItemSize[0]; localX = localX << 1)
    {
        for (uint32_t localY = 1; localY <= maxWorkItemSize[1]; localY = localY << 1)
        {
            // don't run any configurations that exceed the max work group size.
            if(localX * localY > maxWorkGroupSize)
            {
                continue;
            }

            // If we do not divide local sizes into even sections don't run them.
            if(!is2 && (width % localX != 0 || height % localY != 0))
            {
                continue;
            }

            std::cout << "Starting timing for x = " << std::to_string(localX) + " y = " << std::to_string(localY) << std::endl;
            std::vector<double> runTimes;
            auto mapping = std::make_pair(localX, localY);

            for(auto order = MIN_ORDER; order < ORDER/10.0f + MIN_ORDER; order += 0.1f)
            {
                const auto bailout = std::pow(std::pow(FLT_MAX, 1.0f / (order + 2.0f)), 1.0f / 2.0f);

                auto runBuffer = cl::Buffer(zeroState.begin(), zeroState.end(), false);

                // Start the Kernel call.
                const auto startKernelRun = std::chrono::system_clock::now();
                try
                {
                    runKernel(
                        cl::EnqueueArgs(cl::NDRange(width, height), cl::NDRange(localX, localY)),
                        runBuffer,
                        outputPixelBuffer,
                        colorBuffer,
                        numColors,
                        MAX_ITERATIONS,
                        order,
                        bailout
                    );

                    // Wait for the kernel to finish.
                    cl::finish();
                }
                catch (std::runtime_error e)
                {
                    std::cout << "Error running kernels." << e.what() << std::endl;
                    return -1;
                }

                const auto endKernelRun = std::chrono::system_clock::now();
                auto runTime = endKernelRun - startKernelRun;

                runTimes.push_back(static_cast<double>(runTime.count()));
            }
            const auto runTotal = std::accumulate(runTimes.begin(), runTimes.end(), 0.0);
            timingMap[mapping] = runTotal;
        }
    }

    std::cout << "Ending local size optimization." << std::endl;

    auto lowestRunConfig = std::make_pair(1, 1);
    auto lowestRunTime = DBL_MAX;
    for(const auto runMapping : timingMap)
    {
        const auto config = runMapping.first;
        const auto time = runMapping.second;
        if(time < lowestRunTime)
        {
            lowestRunTime = time;
            lowestRunConfig = config;
        }
    }

    std::cout << "Configuration chosen : x = " << std::to_string(lowestRunConfig.first) << " y = " << std::to_string(lowestRunConfig.second) << std::endl;

    // Create the SDL window that we will use.
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Set", 0, 0, width, height, SDL_WINDOW_SHOWN);
    if (win == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create the Renderer that will render our image into the window.
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if(tex == nullptr)
    {
        std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    const auto startTime = std::chrono::system_clock::now();

    std::chrono::duration<double> bailoutCalcTime(0);
    std::chrono::duration<double> copyToKernelTime(0);
    std::chrono::duration<double> kernelRunTime(0);
    std::chrono::duration<double> copyFromKernelTime(0);
    std::chrono::duration<double> textureGenTime(0);
    std::chrono::duration<double> pixelPutTime(0);

    for (auto order = MIN_ORDER; order < ORDER; order += 0.01f)
    {
        // Get the bailout value based off of the want to never get a value to reach infinity.
        // At low orders, the bailout will be high, at high orders the bailout will be low.
        // This prevents oddities where you get black bars showing up in the smoothed filter.
        const auto startBailoutCalc = std::chrono::system_clock::now();
        const auto bailout = std::pow(std::pow(FLT_MAX, 1.0f / (order + 2.0f)), 1.0f/2.0f);
        const auto endBailoutCalc = std::chrono::system_clock::now();
        bailoutCalcTime += endBailoutCalc - startBailoutCalc;

        // Start the Kernel call.

        const auto startCopyToKernelTime = std::chrono::system_clock::now();
        auto runBuffer = cl::Buffer(zeroState.begin(), zeroState.end(), false);
        const auto endCopyToKernelTime = std::chrono::system_clock::now();
        copyToKernelTime += endCopyToKernelTime - startCopyToKernelTime;

        const auto startKernelRun = std::chrono::system_clock::now();
        runKernel(
            cl::EnqueueArgs(cl::NDRange(width, height), cl::NDRange(lowestRunConfig.first, lowestRunConfig.second)),
            runBuffer,
            outputPixelBuffer,
            colorBuffer,
            numColors,
            MAX_ITERATIONS,
            order,
            bailout
            );

        // Wait for the kernel to finish.
        cl::finish();

        const auto endKernelRun = std::chrono::system_clock::now();
        kernelRunTime += endKernelRun - startKernelRun;

        const auto startCopyFromKernelTime = std::chrono::system_clock::now();
        cl::copy(outputPixelBuffer, outputPixels.begin(), outputPixels.end());
        const auto endCopyFromKernelTime = std::chrono::system_clock::now();
        copyFromKernelTime += endCopyFromKernelTime - startCopyFromKernelTime;

        const auto startTextureGenTime = std::chrono::system_clock::now();
        // Lock the texture so we can write to it.
        void* pixels = nullptr;
        auto pitch = 0;
        if(SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0)
        {
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            std::cout << "SDL_LockTexture Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        memcpy(pixels, outputPixels.data(), pitch * height);

        // Unlock the texture so that it updates.
        SDL_UnlockTexture(tex);
        const auto endTextureGenTime = std::chrono::system_clock::now();
        textureGenTime += endTextureGenTime - startTextureGenTime;

        const auto startPixelPut = std::chrono::system_clock::now();
        //First clear the renderer
        SDL_RenderClear(ren);
        //Draw the texture
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        //Update the screen
        SDL_RenderPresent(ren);
        // Get rid of the texture
        //SDL_DestroyTexture(tex);
        const auto endPixelPut = std::chrono::system_clock::now();
        pixelPutTime += endPixelPut - startPixelPut;
    }

    const auto endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> diff = endTime - startTime;

    std::cout << "Run Time Mandelbrot for " << std::to_string(MAX_ITERATIONS) << " = " << diff.count() << " seconds" << std::endl;
    auto otherTime = diff.count();
    std::cout << "Bailout Calculation Time = " << bailoutCalcTime.count() << " seconds" << std::endl;
    otherTime -= bailoutCalcTime.count();
    std::cout << "Copy To Kernel Time = " << copyToKernelTime.count() << " seconds" << std::endl;
    otherTime -= copyToKernelTime.count();
    std::cout << "Kernel Run Time = " << kernelRunTime.count() << " seconds" << std::endl;
    otherTime -= kernelRunTime.count();
    std::cout << "Copy From Kernel Time = " << copyFromKernelTime.count() << " seconds" << std::endl;
    otherTime -= copyFromKernelTime.count();
    std::cout << "Texture Gen Time = " << textureGenTime.count() << " seconds" << std::endl;
    otherTime -= textureGenTime.count();
    std::cout << "Pixel Put Time = " << pixelPutTime.count() << " seconds" << std::endl;
    otherTime -= pixelPutTime.count();
    std::cout << "Other Time = " << otherTime << " seconds" << std::endl;

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    std::cout << "Succeeded" << std::endl;

    return 0;
}