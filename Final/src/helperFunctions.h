#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include <CL/cl.h>

namespace Helper
{
    struct MandelbrotSaveState
    {
        cl_float2 complex;
        cl_float2 constantComplex;
        uint32_t count;
        float adjustedCount;
    };

    static std::vector<std::vector<uint8_t>> gColors;

    std::string slurp(const std::string& fileName);
    double variance(const std::vector<double>& samples);
    double standardDeviation(const std::vector<double>& samples);
    std::pair<double, double> printResults(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times, const std::string& filename);
    std::pair<double, double> printResults(std::vector<std::pair<cl_ulong, cl_ulong>>& times, const std::string& filename);
    uint8_t* convert1To4Channel(uint8_t* oneChannelData, const uint32_t& numPixels);
    uint8_t* convert3To4Channel(uint8_t* threeChannelData, const uint32_t& numPixels);
    std::string clGetErrorString(const int &errorCode);
    bool processClCallStatus(const std::string& callName, const int &errorCode); 
    void setGlobalColors(uint32_t maxCount, std::pair<uint8_t, uint8_t> redPair, std::pair<uint8_t, uint8_t> greenPair, std::pair<uint8_t, uint8_t> bluePair);
    std::vector<uint8_t> getColorHelper(uint32_t count, uint32_t maxCount, std::pair<uint8_t, uint8_t> &redPair, std::pair<uint8_t, uint8_t> &greenPair, std::pair<uint8_t, uint8_t> &bluePair);
    std::vector<uint8_t> getColors(uint32_t count, float adjust);
    std::vector<Helper::MandelbrotSaveState> generateZeroState(float left, float top, float xSide, float ySide, uint32_t xMax, uint32_t yMax);
}
