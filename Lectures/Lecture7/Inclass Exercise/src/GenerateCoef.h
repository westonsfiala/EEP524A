#pragma once
#include <vector>
#include <string>

std::vector<float> generateGaussBlur(const uint32_t &matSize, const float &sigma2);

std::pair<std::vector<float>, std::vector<float>> generateSobelFilters();
