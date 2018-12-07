// Include the c++ wrapper of openCL and enable exceptions
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>
#include <iostream>

#include "helperFunctions.h"

#include <vector>
#include <ostream>
#include <iostream>
#include <SDL.h>
#include <chrono>
#include <algorithm>

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

static const auto MAX_X = 1920;
static const auto MAX_Y = 1080;
static const auto MAX_ITERATIONS = 100;

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0, 0);
static const auto ORDER = 8.5f;

static const float DEFAULT_FIT = 2.5f;
static const auto ESCAPE_NUM = static_cast<float>(std::min<uint32_t>(UINT32_MAX, 1 << 2 * static_cast<int>(ORDER)));

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
        cl::fine_svm_vector<cl_float2>,
        cl::fine_svm_vector<cl_float2>
    >(outputGlobalProgram, OUTPUT_GLOBAL_KERNEL_NAME);

    cl_float2 ones = { 1.0f, 2.0f };
    cl_float2 zeros = { -1.0f, -1.0f };
    const cl::fine_svm_vector<cl_float2> inputVector(10, ones);
    cl::fine_svm_vector<cl_float2> outputVector(10, zeros);

    outputRunKernel(
        cl::EnqueueArgs(cl::NDRange(10)),
        inputVector,
        outputVector
    );

    cl::mapSVM(outputVector);

    // Get the kernel string for the output global ID kernel.
    const auto kernelString = Helper::slurp(MANDELBROT_KERNEL_FILE);
    const std::vector<std::string> programStrings {kernelString};

    cl::Program MandelbrotProgram(programStrings);
    try {
        // ReSharper disable once CppExpressionWithoutSideEffects
        MandelbrotProgram.build("-cl-std=CL2.0");
    }
    catch (...) {
        // Print build info for all devices
        auto buildErr = CL_SUCCESS;
        auto buildInfo = MandelbrotProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
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

    // Create the SDL window that we will use.
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Set", 0, 0, MAX_X, MAX_Y, SDL_WINDOW_SHOWN);
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

    // Get the Screen Ratio
    const auto screenRatio = static_cast<float>(MAX_X) / MAX_Y;

    // Center it and set the zoom level.
    // I don't know why this number is needed, but it centers it.
    const auto left = (-DEFAULT_FIT + 0.28125f) / ZOOM - CENTER.first;
    const auto top = -DEFAULT_FIT / 2 / ZOOM + CENTER.second;
    const auto xSide = 2.5f * screenRatio / ZOOM;
    const auto ySide = 2.5f / ZOOM;

    // Setup the kernel so that when we have the arguments we can run it right away.
    auto runKernel = cl::KernelFunctor<
        cl::Buffer,
        uint32_t,
        cl_float
        >(MandelbrotProgram, MANDELBROT_KERNEL_NAME);


    auto initialState = Helper::generateZeroState(left, top, xSide, ySide, MAX_X, MAX_Y);

    cl::Buffer outputBuffer(initialState.begin(), initialState.end(), false);

    const auto startTime = std::chrono::system_clock::now();

    for (uint32_t i = 5; i < MAX_ITERATIONS; ++i)
    {
        runKernel(
            cl::EnqueueArgs(cl::NDRange(MAX_X, MAX_Y)),
            outputBuffer,
            i,
            ORDER
            );

        cl::copy(outputBuffer, initialState.begin(), initialState.end());

        auto pixelMap = std::vector<uint8_t>();
        for (auto fractalState : initialState)
        {
            auto pixelValue = static_cast<uint8_t>(255 - (255 * fractalState.adjustedCount / i));
            pixelMap.push_back(pixelValue);
            pixelMap.push_back(pixelValue);
            pixelMap.push_back(pixelValue);
            pixelMap.push_back(255);
        }

        // Create our surface from the 
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixelMap.data()), MAX_X, MAX_Y, 32, MAX_X * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

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
    }

    const auto endTime = std::chrono::system_clock::now();

    std::chrono::duration<double> diff = endTime - startTime;

    std::cout << "Run Time Mandelbrot for " << std::to_string(MAX_ITERATIONS) << " = " << diff.count() << " seconds" << std::endl;


    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    std::cout << "Succeeded" << std::endl;

    return 0;
}