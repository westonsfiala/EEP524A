// Include the c++ wrapper of openCL and enable exceptions
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include <CL/cl2.hpp>
#include <iostream>

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

static const auto MAX_ITERATIONS = 25;

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0.0f, 0.0f);
static const auto ORDER = 15.0f;

static const float DEFAULT_FIT = 2.5f;

int main(int argc, char** argv)
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
        if (version.find("OpenCL 1.") != std::string::npos) {

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
            /*
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
            */
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
        // ReSharper disable once CppExpressionWithoutSideEffects
        outputGlobalProgram.build("-cl-std=CL2.0");
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
        cl::Buffer,
        cl::Buffer
    >(outputGlobalProgram, OUTPUT_GLOBAL_KERNEL_NAME);

    cl_float2 ones = { 1.0f, 1.0f };
    cl_float2 negativeOnes = { -1.0f, -1.0f };

    std::vector<cl_float2> inputVector(10, ones);
    std::vector<cl_float2> outputVector(10, negativeOnes);

    const cl::Buffer inputBuffer(inputVector.begin(), inputVector.end(), true);
    const cl::Buffer outputBuffer2(outputVector.begin(), outputVector.end(), false);

    outputRunKernel(
        cl::EnqueueArgs(cl::NDRange(10)),
        inputBuffer,
        outputBuffer2
    );

    cl::copy(outputBuffer2, outputVector.begin(), outputVector.end());

    */

    // Get the kernel string for the mandelbrot kernel.
    const auto kernelString = Helper::slurp(MANDELBROT_KERNEL_FILE);
    const std::vector<std::string> programStrings {kernelString};

    cl::Program mandelbrotProgram(programStrings);
    try {
        // ReSharper disable once CppExpressionWithoutSideEffects
        mandelbrotProgram.build("-cl-std=CL1.2");
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

    std::vector<uint8_t> outputPixels(4 * height*width, 0);

    const cl::Buffer outputPixelBuffer(outputPixels.begin(), outputPixels.end(), false);

    // Set the global colors so that it is a fast lookup.
    Helper::setGlobalColorsFade({});

    auto setColors = Helper::getAssignedColors();
    const auto numColors = static_cast<uint32_t>(setColors.size() / 3);

    const cl::Buffer colorBuffer(setColors.begin(), setColors.end(), true);

    std::vector<uint8_t> pixel(4, 0);

    std::vector<uint8_t> pixelMap(height * width * 4, 0);

    std::map<std::pair<uint32_t, uint32_t>, double> timingMap;

    std::cout << "Starting local size optimization." << std::endl;

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
            if(width % localX != 0 || height % localY != 0)
            {
                continue;
            }
            std::cout << "Starting timing for x = " << std::to_string(localX) + " y = " << std::to_string(localY) << std::endl;
            std::vector<double> runTimes;
            auto mapping = std::make_pair(localX, localY);
            const auto order = 2.0f;
            const auto bailout = std::pow(FLT_MAX, 1.0f / (order + 1.0f));

            for(auto runNum = 0; runNum < 5; ++runNum)
            {
                // Need to reinitialize every time due to the different orders.
                auto initialState = Helper::generateZeroState(left, top, xSide, ySide, width, height);

                const cl::Buffer fractalStateBuffer(initialState.begin(), initialState.end(), false);

                // Start the Kernel call.
                const auto startKernelRun = std::chrono::system_clock::now();
                try
                {
                    runKernel(
                        cl::EnqueueArgs(cl::NDRange(width, height), cl::NDRange(localX, localY)),
                        fractalStateBuffer,
                        outputPixelBuffer,
                        colorBuffer,
                        numColors,
                        MAX_ITERATIONS,
                        order,
                        bailout
                    );
                }
                catch (...)
                {
                    std::cout << "Error running kernels" << std::endl;
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

    const auto startTime = std::chrono::system_clock::now();

    std::chrono::duration<double> kernelRunTime(0);
    std::chrono::duration<double> pixelPutTime(0);

    for (auto order = 1.0f; order < ORDER; order += 0.01f)
    {
        // Get the bailout value based off of the want to never get a value to reach infinity.
        // At low orders, the bailout will be high, at high orders the bailout will be low.
        // This prevents oddities where you get black bars showing up in the smoothed filter.
        const auto bailout = std::pow(FLT_MAX, 1.0f / (order+1.0f));

        // Need to reinitialize every time due to the different orders.
        auto initialState = Helper::generateZeroState(left, top, xSide, ySide, width, height);

        const cl::Buffer fractalStateBuffer(initialState.begin(), initialState.end(), false);

        // Start the Kernel call.
        const auto startKernelRun = std::chrono::system_clock::now();
        runKernel(
            cl::EnqueueArgs(cl::NDRange(width, height), cl::NDRange(lowestRunConfig.first, lowestRunConfig.second)),
            fractalStateBuffer,
            outputPixelBuffer,
            colorBuffer,
            numColors,
            MAX_ITERATIONS,
            order,
            bailout
            );

        cl::copy(outputPixelBuffer, pixelMap.begin(), pixelMap.end());
        const auto endKernelRun = std::chrono::system_clock::now();
        kernelRunTime += endKernelRun - startKernelRun;
        
        // Start putting the pixels onto the screen.
        const auto startPixelPut = std::chrono::system_clock::now();
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixelMap.data()), width, height, 32, width * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (tex == nullptr)
        {
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        //First clear the renderer
        SDL_RenderClear(ren);
        //Draw the texture
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        //Update the screen
        SDL_RenderPresent(ren);
        // Get rid of the texture
        SDL_DestroyTexture(tex);

        const auto endPixelPut = std::chrono::system_clock::now();

        pixelPutTime += endPixelPut - startPixelPut;
    }

    const auto endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> diff = endTime - startTime;

    std::cout << "Run Time Mandelbrot for " << std::to_string(MAX_ITERATIONS) << " = " << diff.count() << " seconds" << std::endl;
    std::cout << "Kernel Run Time = " << kernelRunTime.count() << " seconds" << std::endl;
    std::cout << "Pixel Put Time = " << pixelPutTime.count() << " seconds" << std::endl;

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    std::cout << "Succeeded" << std::endl;

    return 0;
}