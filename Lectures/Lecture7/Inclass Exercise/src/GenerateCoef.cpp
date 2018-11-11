#include "GenerateCoef.h"
#include <cassert>

std::vector<float> generateGaussBlur(const uint32_t &matSize, const float &sigma2)
{
    // What are the max and min values going to be.
    const auto maxVal = 1.0f;
    const auto minVal = -1.0f;

    // Get the step value.
    // linspace(-1, 1, N);
    const auto step = (maxVal - minVal) / (matSize - 1);

    auto value = minVal;

    std::vector<float> xValues;
    xValues.reserve(matSize);
    for (uint32_t i = 0; i < matSize; ++i)
    {
        xValues.push_back(value);
        value += step;
    }

    // Get the equivalent of the meshgrid command.
    // [X, Y] = meshgrid(x);
    std::vector<std::vector<float>> xGrid;
    std::vector<std::vector<float>> yGrid;
    xGrid.reserve(matSize);
    yGrid.reserve(matSize);

    for (uint32_t i = 0; i < matSize; ++i)
    {
        // Create the row.
        xGrid.emplace_back();
        yGrid.emplace_back();
        for (uint32_t j = 0; j < matSize; ++j)
        {
            // Fill the row out.
            xGrid[i].push_back(xValues[i]);
            yGrid[i].push_back(xValues[j]);
        }
    }

    // Get our modifiers.
    const auto amplitude = 1.0f;

    // Generate the actual curve. 
    // F = Ap * exp(-(X. ^ 2 + Y. ^ 2) / (2 * sigma2));
    std::vector<float> xyGrid;
    xyGrid.reserve(matSize * matSize);

    auto fSum = 0.0f;

    for (uint32_t i = 0; i < matSize; ++i)
    {
        for (uint32_t j = 0; j < matSize; ++j)
        {
            // Fill in the row.
            auto fVal = amplitude * exp(-(pow(xGrid[i][j], 2) + pow(yGrid[i][j], 2)) / (2 * sigma2));
            fSum += fVal;
            xyGrid.push_back(fVal);
        }
    }

    // Get the normalization.
    const auto fNorm = 1 / fSum;

    auto normedSum = 0.0f;

    // Normalize the xy_grid.
    for (auto & val : xyGrid)
    {
        const auto normedVal = val * fNorm;
        val = normedVal;
        normedSum += normedVal;
    }

    assert(normedSum > 0.999f && normedSum < 1.001f);

    return xyGrid;
}

std::pair<std::vector<float>, std::vector<float>> generateSobelFilters()
{
    std::vector<float> smoothVector = { 1, 2, 1 };
    std::vector<float> centerDiffVector = { 1, 0, -1 };

    std::vector<float> horizontalMatrix;
    std::vector<float> verticalMatrix;

    for(auto smooth : smoothVector)
    {
        for(auto diff : centerDiffVector)
        {
            horizontalMatrix.push_back(smooth * diff);
        }
    }

    for (auto diff : centerDiffVector)
    {
        for (auto smooth : smoothVector)
        {
            verticalMatrix.push_back(smooth * diff);
        }
    }

    return { horizontalMatrix, verticalMatrix };
}
