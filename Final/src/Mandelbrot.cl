
#include "../../src/clcomplex.h"

struct MandelbrotSaveState
{
    float2 complex;
    float2 constantComplex;
    unsigned int count;
    float adjustedCount;
};


kernel void Mandelbrot (
    global struct MandelbrotSaveState* saveStates,
    global unsigned char* outputPixels,
    global const unsigned char* colors,
    const unsigned int numColors,
    const unsigned int maxCount,
    const float order,
    const float bailout
)
{
    const int xId = get_global_id(0);
    const int yId = get_global_id(1);
    const int xSize = get_global_size(0);

    const int index = yId * xSize + xId;

    struct MandelbrotSaveState saveState = saveStates[index];

    //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);

    while (cabsf(saveState.complex) < bailout && saveState.count < maxCount)
    {
        const float2 power = {order, 0.0f};

        //printf("power = %f %f\n", power.x, power.y);
        //printf("complex = %f %f\n", saveState.complex.x, saveState.complex.y);

        // Perform the next calculation.
        saveState.complex = cpowf(saveState.complex, power) + saveState.constantComplex;

        // Increment count 
        saveState.count++;

        //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);
    }
    
    // Get the index into the output pixels.
    const int pixelIndex = 3*index;

    // See if we reached the max count, if so output black pixels.
    if(saveState.count == maxCount)
    { 
        outputPixels[pixelIndex] = 0;
        outputPixels[pixelIndex+1] = 0;
        outputPixels[pixelIndex+2] = 0;
    }
    // We did not reach maxCount, time to calculate our colors.
    else
    { 
        // Get the value to adjust the count by.
        // This determines how to smoothe between the two adjacent colors.
        const float logZn = (float)log10(cabsf(saveState.complex));
        const float adjust = (float)log10(logZn / log10(bailout)) / log10(order);
        saveState.adjustedCount = 1.0f - adjust;
        //saveState.adjustedCount = 0;

        const int index1 = ((int)floor(saveState.adjustedCount) + saveState.count + 1) % numColors;
        const int index2 = ((int)floor(saveState.adjustedCount) + saveState.count) % numColors;

        //printf("%i %i mod %i -> %i %i\n", (saveState.count + 1), saveState.count, numColors, index1, index2);

        const int colorIndex1 = 3*index1;
        const int colorIndex2 = 3*index2;

        // Get all the colors based off of what was given to us, and the one plus of what was given to us.
        const unsigned char red1 = colors[colorIndex1];
        const unsigned char green1 = colors[colorIndex1 + 1];
        const unsigned char blue1 = colors[colorIndex1 + 2];
        
        const unsigned char red2 = colors[colorIndex2];
        const unsigned char green2 = colors[colorIndex2 + 1];
        const unsigned char blue2 = colors[colorIndex2 + 2];

        // Find the weightings for the smoothing.
        const float weight1 = saveState.adjustedCount;
        const float weight2 = 1 - weight1;

        // Smooth each of the lanes.
        const unsigned char red = (unsigned char)(red1 * weight1 + red2 * weight2);
        const unsigned char green = (unsigned char)(green1 * weight1 + green2 * weight2);
        const unsigned char blue = (unsigned char)(blue1 * weight1 + blue2 * weight2);

        outputPixels[pixelIndex] = red;
        outputPixels[pixelIndex + 1] = green;
        outputPixels[pixelIndex + 2] = blue;
    }

    // Save the current state so that our future kernels can use it.
    saveStates[index] = saveState;
}