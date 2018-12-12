#include "kernelGenerator.h"
#include <sstream>

KernelGenerator::KernelGenerator()
{
}

std::string KernelGenerator::getEnhanceKernelString()
{
    return "";
}

std::string KernelGenerator::getZoomKernelString()
{
    return "";
}

std::string KernelGenerator::getIncreaseOrderString()
{
    std::stringstream kernelString;
    kernelString << R"(#include "../../src/clcomplex.h")" << std::endl;
    kernelString << "" << std::endl;
    kernelString << "struct MandelbrotSaveState" << std::endl;
    kernelString << "{" << std::endl;
    kernelString << "    float2 complex;" << std::endl;
    kernelString << "    float2 constantComplex;" << std::endl;
    kernelString << "    unsigned int count;" << std::endl;
    kernelString << "    float adjustedCount;" << std::endl;
    kernelString << "    float2 padding;" << std::endl;
    kernelString << "};" << std::endl;
    kernelString << "" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "kernel void Mandelbrot(" << std::endl;
    kernelString << "    global struct MandelbrotSaveState* saveStates," << std::endl;
    kernelString << "    global unsigned char* outputPixels," << std::endl;
    kernelString << "    global const unsigned char* colors," << std::endl;
    kernelString << "    const unsigned int numColors," << std::endl;
    kernelString << "    const unsigned int maxCount," << std::endl;
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
    kernelString << "    struct MandelbrotSaveState saveState = saveStates[index];" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    float tempAbs = cabsf(saveState.complex);" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << R"(    //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);)" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    while (tempAbs < bailout && saveState.count < maxCount)" << std::endl;
    kernelString << "    {" << std::endl;
    kernelString << "        const float2 power = { order, 0.0f };" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << R"(        //printf("power = %f %f\n", power.x, power.y);)" << std::endl;
    kernelString << R"(        //printf("complex = %f %f\n", saveState.complex.x, saveState.complex.y);)" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Perform the next calculation." << std::endl;
    kernelString << "        saveState.complex = cpowf(saveState.complex, power) + saveState.constantComplex;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        // Increment count " << std::endl;
    kernelString << "        saveState.count++;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "        tempAbs = cabsf(saveState.complex);" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << R"(        //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);)" << std::endl;
    kernelString << "    }" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    // Get the index into the output pixels." << std::endl;
    kernelString << "    const int pixelIndex = 3 * index;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << "    // See if we reached the max count, if so output black pixels." << std::endl;
    kernelString << "    if (saveState.count == maxCount)" << std::endl;
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
    kernelString << "        saveState.adjustedCount = 1.0f - adjust;" << std::endl;
    kernelString << "        //saveState.adjustedCount = 0;" << std::endl;
    kernelString << "" << std::endl;
    kernelString << "        const int index1 = ((int)floor(saveState.adjustedCount) + saveState.count + 1) % numColors;" << std::endl;
    kernelString << "        const int index2 = ((int)floor(saveState.adjustedCount) + saveState.count) % numColors;" << std::endl;
    kernelString << "        " << std::endl;
    kernelString << R"(        //printf("%i %i mod %i -> %i %i\n", (saveState.count + 1), saveState.count, numColors, index1, index2);)" << std::endl;
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
    kernelString << "        const float weight1 = saveState.adjustedCount;" << std::endl;
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

std::string KernelGenerator::getKernelBase()
{
    return "";
}
