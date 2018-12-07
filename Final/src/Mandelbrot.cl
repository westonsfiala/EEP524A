
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
    const float order
)
{
    float escapeVal = 10000.0f * order;

    int xId = get_global_id(0);
    int yId = get_global_id(1);
    int xSize = get_global_size(0);

    int index = yId * xSize + xId;

    struct MandelbrotSaveState saveState = saveStates[index];

    //printf("Save State = %f %fi : %f %fi : %i : %f\n", saveState.complex.x, saveState.complex.y, saveState.constantComplex.x, saveState.constantComplex.y, saveState.count, saveState.adjustedCount);

    while (cabsf(saveState.complex) < escapeVal && saveState.count < maxCount)
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

    saveState.adjustedCount = (float)saveState.count;

    // Used to avoid floating point issues with points inside the set.
    //if (saveState.count < maxCount)
    //{
    //    const float logZn = (float)log(cabsf(saveState.complex));
    //    const float nu = (float)log(logZn / log(escapeVal) / log(order));
        // Rearranging the potential function.
        // Dividing log_zn by log(2) instead of log(N = 1<<8)
        // because we want the entire palette to range from the
        // center to radius 2, NOT our bailout radius.
    //    saveState.adjustedCount = saveState.count + 1 - nu;
    //}

    saveStates[index] = saveState;
}