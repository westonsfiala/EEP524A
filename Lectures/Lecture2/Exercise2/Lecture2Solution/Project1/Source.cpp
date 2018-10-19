#include <fstream>
#include <vector>
#include <string>

static const std::string input_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture4/Inclass Exercise 1/input_file";
static const std::string output_file_name = "C:/work/GitHub/EEP524A/Lectures/Lecture4/Inclass Exercise 1/output_file";
static const std::vector<int> matrix_sizes = { 128, 256, 512, 1024, 2048 };


using namespace std;



int main() {

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
}
