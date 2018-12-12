// Include the c++ wrapper of openCL and enable exceptions
#include "helperFunctions.h"

#include <iostream>
#include <vector>
#include <ostream>
#include <iostream>
#include <SDL.h>
#include <chrono>
#include <map>
#include <numeric>
#include <assert.h>
#include <float.h>
#include "kernelGenerator.h"

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
static const auto ORDER = 15.0f;

int main(int argc, char** argv)
{
    // Choose the platform. Method sets the default.
    auto is2 = false;
    auto chosenPlatform = Helper::chooseClPlatform(is2);

    if (chosenPlatform() == nullptr)
    {
        std::cout << "No platform has been chosen as default." << std::endl;
        return -1;
    }

    // Choose the device.
    uint32_t maxWorkGroupSize;
    std::vector<uint32_t> maxWorkItemSize;
    Helper::chooseClDevice(maxWorkGroupSize, maxWorkItemSize);

    if(chosenPlatform() == nullptr)
    {
        std::cout << "No device has been chosen as default." << std::endl;
        return -1;
    }

    // Remove when you don't need to do any random testing.
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

    // Kick up an instance of the kernel generator and get the kernel that we want to run.
    auto kernelGen = KernelGenerator(is2, maxWorkGroupSize, maxWorkItemSize, KernelGenerator::Order);

    kernelGen.setWindowSize(width, height);
    kernelGen.setMaxIterations(MAX_ITERATIONS);

    auto optimalLocalSizeOrder = kernelGen.findOptimalLocalSize(10);

    kernelGen.runMandelbrot(15.0f);

    SDL_Quit();

    std::cout << "Succeeded" << std::endl;

    return 0;
}
