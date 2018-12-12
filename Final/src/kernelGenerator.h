#pragma once
#include <string>

class KernelGenerator
{
public:
    KernelGenerator();

    std::string getEnhanceKernelString();
    std::string getZoomKernelString();
    std::string getIncreaseOrderString();

private:
    std::string getKernelBase();
};
