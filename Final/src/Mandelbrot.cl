
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
    const unsigned int maxCount,
    const float order,
    const float bailout
)
{
    int xId = get_global_id(0);
    int yId = get_global_id(1);
    int xSize = get_global_size(0);

    int index = yId * xSize + xId;

    struct MandelbrotSaveState saveState = saveStates[index];

    //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);

    while (cabsf(saveState.complex) < bailout && saveState.count < maxCount)
    {
        float2 power = {order, 0.0f};

        //printf("power = %f %f\n", power.x, power.y);
        //printf("complex = %f %f\n", saveState.complex.x, saveState.complex.y);

        // Perform the next calculation.
        saveState.complex = cpowf(saveState.complex, power) + saveState.constantComplex;

        // Increment count 
        saveState.count++;

        //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);
    }

    // Get the value to adjust the count by.
    const float logZn = (float)log10(cabsf(saveState.complex));
    const float adjust = (float)log10(logZn / log10(bailout)) / log10(order);
    saveState.adjustedCount = 1 - adjust;

    saveStates[index] = saveState;
}