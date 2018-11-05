#include "GenerateCoef.h"
#include <cassert>

std::vector<std::vector<float>> generate_gauss_blur(const uint32_t &mat_size, const uint32_t &sigma2)
{
    // What are the max and min values going to be.
    const auto max_val = 1.0f;
    const auto min_val = -1.0f;

    // Get the step value.
    // linspace(-1, 1, N);
    const auto step = (max_val - min_val) / (mat_size - 1);

    auto value = min_val;

    std::vector<float> x_values;
    x_values.reserve(mat_size);
    for (uint32_t i = 0; i < mat_size; ++i)
    {
        x_values.push_back(value);
        value += step;
    }

    // Get the equivalent of the meshgrid command.
    // [X, Y] = meshgrid(x);
    std::vector<std::vector<float>> x_grid;
    std::vector<std::vector<float>> y_grid;
    x_grid.reserve(mat_size);
    y_grid.reserve(mat_size);

    for (uint32_t i = 0; i < mat_size; ++i)
    {
        // Create the row.
        x_grid.emplace_back();
        y_grid.emplace_back();
        for (uint32_t j = 0; j < mat_size; ++j)
        {
            // Fill the row out.
            x_grid[i].push_back(x_values[i]);
            y_grid[i].push_back(x_values[j]);
        }
    }

    // Get our modifiers.
    const auto amplitude = 1.0f;

    // Generate the actual curve. 
    // F = Ap * exp(-(X. ^ 2 + Y. ^ 2) / (2 * sigma2));
    std::vector<std::vector<float>> xy_grid;
    xy_grid.reserve(mat_size);

    auto f_sum = 0.0f;

    for (uint32_t i = 0; i < mat_size; ++i)
    {
        // Create the row.
        xy_grid.emplace_back();
        for (uint32_t j = 0; j < mat_size; ++j)
        {
            // Fill in the row.
            auto f_val = amplitude * exp(-(pow(x_grid[i][j], 2) + pow(y_grid[i][j], 2)) / (2 * sigma2));
            f_sum += f_val;
            xy_grid[i].push_back(f_val);
        }
    }

    // Get the normalization.
    const auto f_norm = 1 / f_sum;

    auto normed_sum = 0.0f;

    // Normalize the xy_grid.
    for (uint32_t i = 0; i < mat_size; ++i)
    {
        // Create the row.
        for (uint32_t j = 0; j < mat_size; ++j)
        {
            const auto normed_val = xy_grid[i][j] * f_norm;
            xy_grid[i][j] = normed_val;
            normed_sum += normed_val;
        }
    }

    assert(normed_sum > 0.999f && normed_sum < 1.001f);

    return xy_grid;
}
