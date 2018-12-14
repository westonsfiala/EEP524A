#pragma once
#include <string>
#include "helperFunctions.h"

class KernelGenerator
{
public:
    enum MandelbrotType
    {
        Order
    };

    explicit KernelGenerator(bool is2, uint32_t maxGroupSize, std::vector<uint32_t> maxItemSizes, MandelbrotType type);

    void setWindowSize(uint32_t width, uint32_t height);
    void setLocalSize(uint32_t x, uint32_t y);
    void setMaxIterations(uint32_t maxIterations);
    void setKernelPrepend(std::string prepend);

    std::pair<uint32_t, uint32_t> findOptimalLocalSize(uint32_t numRuns);
    uint32_t findOptimalMaxIterations();
    std::vector<double> runMandelbrot(bool display, float order, float stepSize = 0.01f) const;
    static std::vector<std::string> getTimingNames();

private:

    static const std::string KERNEL_NAME;

    struct MandelbrotSaveStateOrder
    {
        cl_float2 constantComplex;
    };

    typedef cl::KernelFunctor<
        cl::Buffer, // FractalState
        cl::Buffer, // OutputPixels
        cl::Buffer, // Colors
        uint32_t, // numColors
        uint32_t, // maxCount
        cl_float, // order
        cl_float // bailout
    > MandelbrotKernel;

    // All the methods relating to the order mandelbrot.
    std::string getIncreaseOrderString() const;
    std::vector<MandelbrotSaveStateOrder> generateZeroStateOrder(float left, float top, float xSide, float ySide) const;
    std::pair<uint32_t, uint32_t> findOptimalLocalSizeOrder(uint32_t numRuns);
    uint32_t findOptimalMaxIterationsOrder();
    MandelbrotKernel prepareRunStateOrder(cl::Buffer& fractalState, cl::Buffer& outputPixels, cl::Buffer& colors, uint32_t& numColors) const;
    std::vector<double> runMandelbrotOrder(bool display, float order, float stepSize) const;

    MandelbrotKernel getKernelFunctor(const std::string& kernelString) const;

    std::vector<double> runKernelOrder(MandelbrotKernel kernel, bool showVisuals,
                                       float maxOrder, float stepSize, cl::Buffer fractalState, cl::Buffer outputPixels, cl::Buffer colors, uint32_t numColors) const;

    bool mIs2;
    uint32_t mMaxGroupSize;
    std::vector<uint32_t> mMaxItemSizes;
    MandelbrotType mMandelbrotType;

    std::pair<uint32_t, uint32_t> mWindowSize;
    std::pair<uint32_t, uint32_t> mLocalSize;
    uint32_t mMaxIterations;

    std::string mPrepend;
};
