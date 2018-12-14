// Include the c++ wrapper of openCL and enable exceptions
#include "helperFunctions.h"
#include "kernelGenerator.h"

#include <iostream>
#include <SDL.h>
#include <map>

/*************************************************************************
* CHANGE THE FOLLOWING LINES TO WHERE EVER YOU HAVE EVERYTHING MAPPED TO.
*************************************************************************/
//Set the output directories that need to be used.
static const std::string BASE_DIRECTORY = "C:/work/GitHub/EEP524A/Final/";

static const std::string OUTPUT_DIRECTORY = BASE_DIRECTORY + "output/";
static const std::string RESULTS_FILE = OUTPUT_DIRECTORY + "timingResults.csv";

static const std::string SRC_DIRECTORY = BASE_DIRECTORY + "src/";
static const std::string MANDELBROT_KERNEL_NAME = "Mandelbrot";
static const std::string MANDELBROT_KERNEL_FILE = SRC_DIRECTORY + MANDELBROT_KERNEL_NAME + ".cl";
static const std::string CLCOMPLEX_HEADER_FILE = SRC_DIRECTORY + "clcomplex.h";

static const auto MAX_ITERATIONS = 1000;
static const auto ORDER = 11.0f;

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

    if (chosenPlatform() == nullptr)
    {
        std::cout << "No device has been chosen as default." << std::endl;
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

    // Kick up an instance of the kernel generator and get the kernel that we want to run.
    auto kernelGen = KernelGenerator(is2, maxWorkGroupSize, maxWorkItemSize, KernelGenerator::Order);

    const auto clcomplexString = Helper::slurp(CLCOMPLEX_HEADER_FILE);

    kernelGen.setKernelPrepend(clcomplexString);

    kernelGen.setWindowSize(width, height);

    //kernelGen.findOptimalLocalSize(5);
    //kernelGen.findOptimalMaxIterations();

    std::map<uint32_t, std::vector<double>> runTimeMap;
    
    // Loop over 1 -> max iterations and print out all the results.
    for(auto i = 1; i <= MAX_ITERATIONS; ++i)
    {
        // No matter the order, run the kernel 100 times per display.
        const auto stepSize = (ORDER - 1.0f) / 100;

        // Set the number of iterations to calculate to.
        kernelGen.setMaxIterations(i);

        // Run the kernel and get the timing report.
        const auto runTimes = kernelGen.runMandelbrot(true, ORDER, stepSize);

        runTimeMap[i] = runTimes;
    }

    Helper::printResults(runTimeMap, kernelGen.getTimingNames(), RESULTS_FILE);

    SDL_Quit();

    std::cout << "Succeeded" << std::endl;

    return 0;
}
