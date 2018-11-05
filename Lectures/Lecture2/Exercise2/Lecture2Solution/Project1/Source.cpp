#include <fstream>
#include <vector>
#include <string>
#include <valarray>
#include <cassert>

//static const std::string input_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture4/Inclass Exercise 1/input_file";
//static const std::string output_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture4/Inclass Exercise 1/output_file";
//static const std::vector<int> matrix_sizes = { 128, 256, 512, 1024, 2048 };

static const std::string output_file_name = "C:/work/GitHub/EEP524A/Homeworks/Homework4/filterCoef_";

using namespace std;

std::vector<std::vector<float>> generate_gauss_blur(const uint32_t mat_size, const uint32_t sigma2)
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

int main() {

    /*
    for(auto matrix_size : matrix_sizes)
    {
        ofstream myfile;

        const auto input_filename = input_file_name + to_string(matrix_size) + ".bin";

        myfile.open(input_filename, ios::out | ios::binary);

        std::vector<float> stored_matrix;
        stored_matrix.reserve(matrix_size*matrix_size);

        for(auto i = 0; i < matrix_size; ++i)
        {
            for(auto j = 0; j < matrix_size; ++j)
            {
                stored_matrix.push_back(1.0f);
            }
        }

        myfile.write(reinterpret_cast<char*>(stored_matrix.data()), sizeof(float) * stored_matrix.size());

        myfile.close();

        std::vector<float> output_matrix;
        output_matrix.reserve(matrix_size*matrix_size);

        for (auto i = 0; i < matrix_size; ++i)
        {
            for (auto j = 0; j < matrix_size; ++j)
            {
                auto tmp = 0.0f;

                for (auto k = 0; k < matrix_size; ++k)
                {
                    tmp += stored_matrix.at(i*matrix_size + k) * stored_matrix.at(k*matrix_size + j);
                }

                output_matrix.push_back(tmp);
            }
        }

        const auto output_filename = output_file_name + to_string(matrix_size) + ".bin";

        myfile.open(output_filename, ios::out | ios::binary);

        myfile.write(reinterpret_cast<char*>(output_matrix.data()), sizeof(float) * output_matrix.size());

        myfile.close();

    }
    return 0;
    */

    // Generate coefficients for an NxN Gaussian Blur Filter stencil
    const auto N = 9;
    const auto sigma2 = 1.2f;

    auto xy_grid = generate_gauss_blur(N, sigma2);

    ofstream myfile;

    const auto output_filename = output_file_name + std::to_string(N) + "x" + std::to_string(N) + "_sigma" + std::to_string(sigma2) + ".csv";

    myfile.open(output_filename);

    for(auto row : xy_grid)
    {
        std::string row_string;
        for(auto column : row)
        {
            row_string += std::to_string(column) + ", ";
        }
        // Get rid of the ending ", ".
        row_string.pop_back();
        row_string.pop_back();
        myfile << row_string << std::endl;
    }

    myfile.close();

    return 0;
}
