#pragma once
#include <vector>
#include <string>

static const std::string output_file_name = "C:/work/GitHub/EEP524A/Homeworks/Homework4/filterCoef_";

std::vector<std::vector<float>> generate_gauss_blur(const uint32_t &mat_size, const uint32_t &sigma2);
