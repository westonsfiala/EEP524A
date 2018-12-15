#include "kernelGenerator.h"
#include <sstream>
#include <iostream>
#include <map>
#include <numeric>
#include <SDL.h>

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0.0f, 0.0f);
static const auto MIN_ORDER = 1.0f;
static const float DEFAULT_FIT = 2.5f;

const std::string KernelGenerator::KERNEL_NAME = "Mandelbrot";

KernelGenerator::KernelGenerator(const bool is2, const uint32_t maxGroupSize, const std::vector<uint32_t> maxItemSizes, MandelbrotType type)
{
    // Set the values that are passed by the constructor.
    mIs2 = is2;
    mMaxGroupSize = maxGroupSize;
    mMaxItemSizes = maxItemSizes;
    mMandelbrotType = type;

    // Set the values that are set by methods to some good enough values.
    mWindowSize = {512, 512};
    mLocalSize = {4, 4};
    mMaxIterations = 100;
    mNumColors = 0;
}

void KernelGenerator::setWindowSize(uint32_t width, uint32_t height)
{
    mWindowSize = {width, height};
}

void KernelGenerator::setLocalSize(uint32_t x, uint32_t y)
{
    mLocalSize = {x, y};
}

void KernelGenerator::setMaxIterations(const uint32_t maxIterations)
{
    mMaxIterations = maxIterations;
}

void KernelGenerator::setKernelPrepend(std::string prepend)
{
    mPrepend = prepend;
}

std::pair<uint32_t, uint32_t> KernelGenerator::findOptimalLocalSize(const uint32_t numRuns)
{
    // If we have the type 'order' run the optimizations.
    if (mMandelbrotType == Order)
    {
        return findOptimalLocalSizeOrder(numRuns);
    }

    // No recognized type, return whatever we have right now.
    return mLocalSize;
}

uint32_t KernelGenerator::findOptimalMaxIterations()
{
    // If we have the type 'order' run the optimizations.
    if (mMandelbrotType == Order)
    {
        return findOptimalMaxIterationsOrder();
    }

    // No recognized type, return whatever we have right now.
    return mMaxIterations;
}

std::vector<double> KernelGenerator::runMandelbrot(bool display, const float order, const float stepSize)
{
    // If we have the type 'order' run the program.
    if (mMandelbrotType == Order)
    {
        return runMandelbrotOrder(display, order, stepSize);
    }

    return {};
}

std::vector<std::string> KernelGenerator::getTimingNames()
{
    std::vector<std::string> names;
    names.push_back("Total");
    names.push_back("Bailout Calculation");
    names.push_back("Kernel Enqueue");
    names.push_back("Kernel Run");
    names.push_back("Texture Generation");
    names.push_back("Unmap Buffer");
    names.push_back("Pixel Display");
    names.push_back("Frames Per Second");
    names.push_back("Overhead");
    return names;
}

std::string KernelGenerator::getIncreaseOrderString() const
{
    std::stringstream kernelString;
    kernelString << mPrepend << std::endl;
    kernelString << "struct MandelbrotInitialStateOrder" << std::endl;
    kernelString << "{" << std::endl;
    kernelString << "    float2 constantComplex;" << std::endl;
    kernelString << "};" << std::endl << std::endl;
    kernelString << "kernel void " << KERNEL_NAME << "(" << std::endl;
    kernelString << "    global const struct MandelbrotInitialStateOrder* initialStates," << std::endl;
    kernelString << "    global unsigned char* outputPixels," << std::endl;
    kernelString << "    global const unsigned char* colors," << std::endl;
    kernelString << "    const float order," << std::endl;
    kernelString << "    const float bailout" << std::endl;
    kernelString << ")" << std::endl;
    kernelString << "{" << std::endl;
    kernelString << "    const int xId = get_global_id(0);" << std::endl;
    kernelString << "    const int yId = get_global_id(1);" << std::endl;
    kernelString << "    const int xSize = get_global_size(0);" << std::endl;
    kernelString << "    " << std::endl;
    kernelString << "    const int index = yId * xSize + xId;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    struct MandelbrotInitialStateOrder initialState = initialStates[index];" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    float2 complexVal = initialState.constantComplex;" << std::endl;
    kernelString << "    unsigned int count = 1;" << std::endl;
    kernelString << "    float tempAbs = cabsf(complexVal);" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    while (tempAbs < bailout && count < " << std::to_string(mMaxIterations) << ")" << std::endl;
    kernelString << "    {" << std::endl;
    kernelString << "        const float2 power = { order, 0.0f };" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Perform the next calculation." << std::endl;
    kernelString << "        complexVal = cpowf(complexVal, power) + initialState.constantComplex;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Increment count " << std::endl;
    kernelString << "        count++;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        tempAbs = cabsf(complexVal);" << std::endl;
    kernelString << "    }" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    // Get the index into the output pixels." << std::endl;
    kernelString << "    const int pixelIndex = 3 * index;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    // See if we reached the max count, if so output black pixels." << std::endl;
    kernelString << "    if (count == " << std::to_string(mMaxIterations) << ")" << std::endl;
    kernelString << "    {" << std::endl;
    kernelString << "        outputPixels[pixelIndex] = 0;" << std::endl;
    kernelString << "        outputPixels[pixelIndex + 1] = 0;" << std::endl;
    kernelString << "        outputPixels[pixelIndex + 2] = 0;" << std::endl;
    kernelString << "    }" << std::endl;
    kernelString << "    // We did not reach maxCount, time to calculate our colors." << std::endl;
    kernelString << "    else" << std::endl;
    kernelString << "    {" << std::endl;
    kernelString << "        // Get the value to adjust the count by." << std::endl;
    kernelString << "        // This determines how to smooth between the two adjacent colors." << std::endl;
    kernelString << "        const float logZn = (float)log10(tempAbs);" << std::endl;
    kernelString << "        const float adjust = (float)log10(logZn / log10(bailout)) / log10(order);" << std::endl;
    kernelString << "        float adjustedCount = 1.0f - adjust;" << std::endl;
    kernelString << "" << std::endl;
    kernelString << "        const int index1 = ((int)floor(adjustedCount) + count + 1) % " << std::to_string(mNumColors) << ";" << std::endl;
    kernelString << "        const int index2 = ((int)floor(adjustedCount) + count) % " << std::to_string(mNumColors) << ";" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        const int colorIndex1 = 3 * index1;" << std::endl;
    kernelString << "        const int colorIndex2 = 3 * index2;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Get all the colors based off of what was given to us, and the one plus of what was given to us." << std::endl;
    kernelString << "        const unsigned char red1 = colors[colorIndex1];" << std::endl;
    kernelString << "        const unsigned char green1 = colors[colorIndex1 + 1];" << std::endl;
    kernelString << "        const unsigned char blue1 = colors[colorIndex1 + 2];" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        const unsigned char red2 = colors[colorIndex2];" << std::endl;
    kernelString << "        const unsigned char green2 = colors[colorIndex2 + 1];" << std::endl;
    kernelString << "        const unsigned char blue2 = colors[colorIndex2 + 2];" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Find the weightings for the smoothing." << std::endl;
    kernelString << "        const float weight1 = adjustedCount;" << std::endl;
    kernelString << "        const float weight2 = 1 - weight1;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Smooth each of the lanes." << std::endl;
    kernelString << "        const unsigned char red = (unsigned char)(red1 * weight1 + red2 * weight2);" << std::endl;
    kernelString << "        const unsigned char green = (unsigned char)(green1 * weight1 + green2 * weight2);" << std::endl;
    kernelString << "        const unsigned char blue = (unsigned char)(blue1 * weight1 + blue2 * weight2);" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        outputPixels[pixelIndex] = red;" << std::endl;
    kernelString << "        outputPixels[pixelIndex + 1] = green;" << std::endl;
    kernelString << "        outputPixels[pixelIndex + 2] = blue;" << std::endl;
    kernelString << "    }" << std::endl;
    kernelString << "}" << std::endl;

    return kernelString.str();
}

std::vector<KernelGenerator::MandelbrotSaveStateOrder> KernelGenerator::generateZeroStateOrder(const float left, const float top, const float xSide, const float ySide) const
{
    std::vector<MandelbrotSaveStateOrder> initialStates;
    // setting up the xScale and yScale 
    const auto xScale = xSide / mWindowSize.first;
    const auto yScale = ySide / mWindowSize.second;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (uint32_t y = 0; y < mWindowSize.second; y++)
    {
        for (uint32_t x = 0; x < mWindowSize.first; x++)
        {
            MandelbrotSaveStateOrder initialState;
            initialState.constantComplex = {x * xScale + left, y * yScale + top};

            initialStates.push_back(initialState);
        }
    }

    return initialStates;
}

std::pair<uint32_t, uint32_t> KernelGenerator::findOptimalLocalSizeOrder(const uint32_t numRuns)
{
    cl::Buffer zeroStateBuffer;
    cl::Buffer outputPixelBuffer;
    cl::Buffer colorBuffer;
    const auto kernel = prepareRunStateOrder(zeroStateBuffer, outputPixelBuffer, colorBuffer);

    // Prepare for the timing analysis
    std::map<std::pair<uint32_t, uint32_t>, double> timingMap;
    std::cout << "Starting local size optimization." << std::endl;

    // Start looping over all of the valid local item sizes.
    for (uint32_t localX = 1; localX <= mMaxItemSizes[0]; localX = localX << 1)
    {
        for (uint32_t localY = 1; localY <= mMaxItemSizes[1]; localY = localY << 1)
        {
            // don't run any configurations that exceed the max work group size.
            if (localX * localY > mMaxGroupSize)
            {
                continue;
            }

            // 1x1 is the slowest always, don't even try it.
            if (localX == 1 && localY == 1)
            {
                continue;
            }

            // If we do not divide local sizes into even sections don't run them.
            if (!mIs2 && (mWindowSize.first % localX != 0 || mWindowSize.second % localY != 0))
            {
                continue;
            }

            std::cout << "Starting timing for x = " << std::to_string(localX) + " y = " << std::to_string(localY) << std::endl;
            std::vector<double> runTimes;
            mLocalSize = {localX, localY};

            for (uint32_t run = 0; run < numRuns; run++)
            {
                const auto runTime = runKernelOrder(kernel, false, 2.0f, 1.0f, zeroStateBuffer, outputPixelBuffer, colorBuffer);
                runTimes.push_back(static_cast<double>(runTime[0]));
            }

            const auto runTotal = std::accumulate(runTimes.begin(), runTimes.end(), 0.0);
            timingMap[mLocalSize] = runTotal;
        }
    }

    std::cout << "Ending local size optimization." << std::endl;

    auto lowestRunConfig = std::make_pair(1, 1);
    auto lowestRunTime = DBL_MAX;
    for (const auto runMapping : timingMap)
    {
        const auto config = runMapping.first;
        const auto time = runMapping.second;
        if (time < lowestRunTime)
        {
            lowestRunTime = time;
            lowestRunConfig = config;
        }
    }

    std::cout << "Configuration chosen : x = " << std::to_string(lowestRunConfig.first) << " y = " << std::to_string(lowestRunConfig.second) << std::endl;

    mLocalSize = lowestRunConfig;

    return lowestRunConfig;
}

uint32_t KernelGenerator::findOptimalMaxIterationsOrder()
{
    cl::Buffer zeroStateBuffer;
    cl::Buffer outputPixelBuffer;
    cl::Buffer colorBuffer;
    const auto kernel = prepareRunStateOrder(zeroStateBuffer, outputPixelBuffer, colorBuffer);

    auto runtime = 0.0;
    auto lastRuntime = 0.0;
    mMaxIterations = 25;

    std::cout << "Starting max iteration optimization" << std::endl;
    // Wait till we know we will be taking time processing the kernel before we stop optimizing.
    while ((runtime + lastRuntime) / 2.0 < 0.25)
    {
        std::cout << "Starting timing for max iterations = " << std::to_string(mMaxIterations) << std::endl;
        auto runTimes = runKernelOrder(kernel, false, 2.0f, 1.0f, zeroStateBuffer, outputPixelBuffer, colorBuffer);

        lastRuntime = runtime;
        // The third runtime is the kernel runtime.
        runtime = runTimes[3];

        mMaxIterations += 5;
    }

    std::cout << "Ending max iteration optimization." << std::endl;
    std::cout << "Iterations chosen = " << std::to_string(mMaxIterations) << std::endl;

    return mMaxIterations;
}

KernelGenerator::MandelbrotKernel KernelGenerator::prepareRunStateOrder(cl::Buffer& fractalState, cl::Buffer& outputPixels, cl::Buffer& colors)
{
    auto kernel = getKernelFunctor(getIncreaseOrderString());
    const auto width = mWindowSize.first;
    const auto height = mWindowSize.second;

    // Get the Screen Ratio
    const auto screenRatio = static_cast<float>(width) / height;

    // Center it and set the zoom level.
    // I don't know why this number is needed, but it centers it.
    const auto left = (-DEFAULT_FIT + 0.28125f) / ZOOM - CENTER.first;
    const auto top = -DEFAULT_FIT / 2 / ZOOM + CENTER.second;
    const auto xSide = 2.5f * screenRatio / ZOOM;
    const auto ySide = 2.5f / ZOOM;

    // Prepare the inputs to the kernel.

    // ZeroState buffer 
    auto zeroState = generateZeroStateOrder(left, top, xSide, ySide);
    fractalState = cl::Buffer(zeroState.begin(), zeroState.end(), true);

    // Output Pixel buffer
    std::vector<cl_char> pixelVector(height * width * 3, 0);
    outputPixels = cl::Buffer(pixelVector.begin(), pixelVector.end(), false);

    // Color Buffer
    auto colorVector = Helper::setGlobalColorsFade({});
    colors = cl::Buffer(colorVector.begin(), colorVector.end(), true);

    // Number of colors
    mNumColors = static_cast<uint32_t>(colorVector.size() / 3);

    return kernel;
}

std::vector<double> KernelGenerator::runMandelbrotOrder(const bool display, const float order, const float stepSize)
{
    cl::Buffer zeroStateBuffer;
    cl::Buffer outputPixelBuffer;
    cl::Buffer colorBuffer;
    const auto kernel = prepareRunStateOrder(zeroStateBuffer, outputPixelBuffer, colorBuffer);

    return runKernelOrder(kernel, display, order, stepSize, zeroStateBuffer, outputPixelBuffer, colorBuffer);
}

cl::KernelFunctor<cl::Buffer, cl::Buffer, cl::Buffer, cl_float, cl_float> KernelGenerator::getKernelFunctor(const std::string& kernelString) const
{
    // Get the kernel string for the mandelbrot kernel.
    const std::vector<std::string> programStrings{kernelString};

    cl::Program mandelbrotProgram(programStrings);
    try
    {
        std::string buildString;
        if (mIs2)
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
    catch (cl::Error e)
    {
        // Print build info for all devices
        auto buildErr = CL_SUCCESS;
        auto buildInfo = mandelbrotProgram.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto& pair : buildInfo)
        {
            std::cerr << pair.second << std::endl << std::endl;
        }
        throw std::runtime_error("Failed to build program, check log.");
    }

    // Setup the kernel so that when we have the arguments we can run it right away.
    auto kernel = cl::KernelFunctor<
        cl::Buffer, // FractalState
        cl::Buffer, // OutputPixels
        cl::Buffer, // Colors
        cl_float, // order
        cl_float // bailout
    >(mandelbrotProgram, KERNEL_NAME);

    return kernel;
}

std::vector<double> KernelGenerator::runKernelOrder(MandelbrotKernel kernel, const bool showVisuals, const float maxOrder, const float stepSize,
                                                    const cl::Buffer fractalState, const cl::Buffer outputPixels, const cl::Buffer colors) const
{
    cl::Event myEvent;
    SDL_Window* win;

    const auto totalPixelSize = mWindowSize.first * mWindowSize.second * 3;

    // Get the command queue so we can enqueue our mappings right after the run.
    auto commandQueue = cl::CommandQueue::getDefault();

    uint8_t* pixelPointer2 = nullptr;

    cl::Event waitEvent1;
    cl::Event waitEvent2;

    // Create a second buffer so that we can double buffer the kernel keeping its runtime even higher.
    const auto doubleBufferPixels(outputPixels);

    // If we have visuals, actually show the window.
    if (!showVisuals)
    {
        // Create the SDL window that we will use.
        win = SDL_CreateWindow("Mandelbrot Set", 0, 0, mWindowSize.first, mWindowSize.second, SDL_WINDOW_HIDDEN);
        if (win == nullptr)
        {
            std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return {};
        }
    }
    else
    {
        // Create the SDL window that we will use.
        win = SDL_CreateWindow("Mandelbrot Set", 0, 0, mWindowSize.first, mWindowSize.second, SDL_WINDOW_SHOWN);
        if (win == nullptr)
        {
            std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return {};
        }
    }

    // Create the Renderer that will render our image into the window.
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return {};
    }

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, mWindowSize.first, mWindowSize.second);
    if (tex == nullptr)
    {
        std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return {};
    }

    // Start the timing analysis.
    const auto startTime = std::chrono::system_clock::now();
    auto startSubTime = std::chrono::system_clock::now();
    auto endSubTime = std::chrono::system_clock::now();

    std::chrono::duration<double> bailoutCalcTime(0);
    std::chrono::duration<double> kernelQueueTime(0);
    std::chrono::duration<double> kernelRunTime(0);
    std::chrono::duration<double> textureGenTime(0);
    std::chrono::duration<double> unmapBufferTime(0);
    std::chrono::duration<double> pixelPutTime(0);

    bool firstRun = true;

    // Run the kernel.
    for (auto order = MIN_ORDER; order < maxOrder; order += stepSize)
    {
        // The first run, we don't have the double buffer running.
        if (!firstRun)
        {
            // Wait for the double buffer to finish so that we can kick off the next kernel.
            startSubTime = std::chrono::system_clock::now();
            // ReSharper disable once CppExpressionWithoutSideEffects
            waitEvent2.wait();
            endSubTime = std::chrono::system_clock::now();
            kernelRunTime += endSubTime - startSubTime;
        }

        // Get the bailout value based off of the want to never get a value to reach infinity.
        // At low orders, the bailout will be high, at high orders the bailout will be low.
        // This prevents oddities where you get black/single color bars showing up in the smoothed filter.
        startSubTime = std::chrono::system_clock::now();
        const auto bailout = std::pow(std::pow(FLT_MAX, 1.0f / (order + 2.0f)), 1.0f / 2.0f);
        endSubTime = std::chrono::system_clock::now();
        bailoutCalcTime += endSubTime - startSubTime;

        startSubTime = std::chrono::system_clock::now();
        kernel(
            cl::EnqueueArgs(cl::NDRange(mWindowSize.first, mWindowSize.second), cl::NDRange(mLocalSize.first, mLocalSize.second)),
            fractalState,
            outputPixels,
            colors,
            order,
            bailout
        );
        uint8_t* pixelPointer1 = static_cast<uint8_t*>(commandQueue.enqueueMapBuffer(outputPixels, false, CL_MAP_READ, 0, totalPixelSize, nullptr, &waitEvent1));
        endSubTime = std::chrono::system_clock::now();
        kernelQueueTime += endSubTime - startSubTime;

        // In the first run we don't have the double buffer running.
        if (!firstRun)
        {
            startSubTime = std::chrono::system_clock::now();
            // Lock the texture so we can write to it.
            void* pixels = nullptr;
            auto pitch = 0;
            if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0)
            {
                SDL_DestroyTexture(tex);
                SDL_DestroyRenderer(ren);
                SDL_DestroyWindow(win);
                std::cout << "SDL_LockTexture Error: " << SDL_GetError() << std::endl;
                SDL_Quit();
                return {};
            }

            // Push the pixels to the texture.
            memcpy(pixels, pixelPointer2, pitch * mWindowSize.second);

            // Unlock the texture so that it updates.
            SDL_UnlockTexture(tex);
            endSubTime = std::chrono::system_clock::now();
            textureGenTime += endSubTime - startSubTime;

            startSubTime = std::chrono::system_clock::now();
            // ReSharper disable once CppExpressionWithoutSideEffects
            commandQueue.enqueueUnmapMemObject(doubleBufferPixels, pixelPointer2);
            endSubTime = std::chrono::system_clock::now();
            unmapBufferTime += endSubTime - startSubTime;

            startSubTime = std::chrono::system_clock::now();
            //First clear the renderer
            SDL_RenderClear(ren);
            //Draw the texture
            SDL_RenderCopy(ren, tex, nullptr, nullptr);
            //Update the screen
            SDL_RenderPresent(ren);
            endSubTime = std::chrono::system_clock::now();
            pixelPutTime += endSubTime - startSubTime;
        }

        // Wait for the first kernel to finish.
        startSubTime = std::chrono::system_clock::now();
        // ReSharper disable once CppExpressionWithoutSideEffects
        waitEvent1.wait();
        endSubTime = std::chrono::system_clock::now();
        kernelRunTime += endSubTime - startSubTime;

        // Kick off the second kernel to keep processing while we display the data from the first.
        order += stepSize;

        // Don't kick it off if we are going to process something that we don't want.
        if (order < maxOrder)
        {
            startSubTime = std::chrono::system_clock::now();
            const auto bailout2 = std::pow(std::pow(FLT_MAX, 1.0f / (order + 2.0f)), 1.0f / 2.0f);
            endSubTime = std::chrono::system_clock::now();
            bailoutCalcTime += endSubTime - startSubTime;

            startSubTime = std::chrono::system_clock::now();
            kernel(
                cl::EnqueueArgs(cl::NDRange(mWindowSize.first, mWindowSize.second), cl::NDRange(mLocalSize.first, mLocalSize.second)),
                fractalState,
                doubleBufferPixels,
                colors,
                order,
                bailout2
            );
            pixelPointer2 = static_cast<uint8_t*>(commandQueue.enqueueMapBuffer(doubleBufferPixels, false, CL_MAP_READ, 0, totalPixelSize, nullptr, &waitEvent2));
            endSubTime = std::chrono::system_clock::now();
            kernelQueueTime += endSubTime - startSubTime;
        }

        // Lock the texture so we can write to it.
        startSubTime = std::chrono::system_clock::now();
        void* pixels = nullptr;
        auto pitch = 0;
        if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0)
        {
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            std::cout << "SDL_LockTexture Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return {};
        }

        // Copy the pixel data to the texture.
        memcpy(pixels, pixelPointer1, pitch * mWindowSize.second);

        // Unlock the texture so that it updates.
        SDL_UnlockTexture(tex);
        endSubTime = std::chrono::system_clock::now();
        textureGenTime += endSubTime - startSubTime;

        startSubTime = std::chrono::system_clock::now();
        // ReSharper disable once CppExpressionWithoutSideEffects
        commandQueue.enqueueUnmapMemObject(outputPixels, pixelPointer1);
        endSubTime = std::chrono::system_clock::now();
        unmapBufferTime += endSubTime - startSubTime;

        startSubTime = std::chrono::system_clock::now();
        //Clear the renderer
        //SDL_RenderClear(ren);
        //Draw the texture
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        //Update the screen
        SDL_RenderPresent(ren);
        endSubTime = std::chrono::system_clock::now();
        pixelPutTime += endSubTime - startSubTime;

        firstRun = false;
    }

    const auto endTime = std::chrono::system_clock::now();

    std::vector<double> timings;

    std::chrono::duration<double> diff = endTime - startTime;

    // Get the frames per second.
    const auto fps = (maxOrder - MIN_ORDER) / stepSize / diff.count();

    timings.push_back(diff.count());
    timings.push_back(bailoutCalcTime.count());
    timings.push_back(kernelQueueTime.count());
    timings.push_back(kernelRunTime.count());
    timings.push_back(textureGenTime.count());
    timings.push_back(unmapBufferTime.count());
    timings.push_back(pixelPutTime.count());
    timings.push_back(fps);

    if (showVisuals)
    {
        std::cout << "Run Time Mandelbrot for " << std::to_string(mMaxIterations) << " = " << diff.count() << " seconds" << std::endl;
        auto otherTime = diff.count();

        std::cout << "Bailout Calculation Time = " << bailoutCalcTime.count() << " seconds" << std::endl;
        otherTime -= bailoutCalcTime.count();

        std::cout << "Kernel Queue Time = " << kernelQueueTime.count() << " seconds" << std::endl;
        otherTime -= kernelQueueTime.count();

        std::cout << "Kernel Run Time = " << kernelRunTime.count() << " seconds" << std::endl;
        otherTime -= kernelRunTime.count();

        std::cout << "Texture Gen Time = " << textureGenTime.count() << " seconds" << std::endl;
        otherTime -= textureGenTime.count();

        std::cout << "Unmap Buffer Time = " << unmapBufferTime.count() << " seconds" << std::endl;
        otherTime -= unmapBufferTime.count();

        std::cout << "Pixel Put Time = " << pixelPutTime.count() << " seconds" << std::endl;
        otherTime -= pixelPutTime.count();

        std::cout << "Other Time = " << otherTime << " seconds" << std::endl;
        timings.push_back(otherTime);

        std::cout << "Average frames per second = " << std::to_string(fps) << std::endl;
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);

    return timings;
}
