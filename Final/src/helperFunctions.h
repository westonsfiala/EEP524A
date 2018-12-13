#pragma once
#include <string>
#include <vector>
#include <Windows.h>

#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_MINIMUM_OPENCL_VERSION 200
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/cl2.hpp>

namespace Helper
{
    std::string slurp(const std::string& fileName);
    double variance(const std::vector<double>& samples);
    double standardDeviation(const std::vector<double>& samples);
    std::pair<double, double> printResults(std::vector<std::pair<LARGE_INTEGER, LARGE_INTEGER>>& times, const std::string& filename);
    std::pair<double, double> printResults(std::vector<std::pair<cl_ulong, cl_ulong>>& times, const std::string& filename);
    uint8_t* convert1To4Channel(uint8_t* oneChannelData, const uint32_t& numPixels);
    uint8_t* convert3To4Channel(uint8_t* threeChannelData, const uint32_t& numPixels);
    std::string clGetErrorString(const int &errorCode);
    bool processClCallStatus(const std::string& callName, const int &errorCode); 

    std::vector<cl_char> setGlobalColorsPattern(uint32_t maxCount, std::pair<uint8_t, uint8_t> redPair, std::pair<uint8_t, uint8_t> greenPair, std::pair<uint8_t, uint8_t> bluePair);
    std::vector<uint8_t> getColorHelper(uint32_t count, uint32_t maxCount, std::pair<uint8_t, uint8_t> &redPair, std::pair<uint8_t, uint8_t> &greenPair, std::pair<uint8_t, uint8_t> &bluePair);

    std::vector<cl_char> setGlobalColorsFade(std::vector<std::vector<uint8_t>> colorList);

    cl::Platform chooseClPlatform(bool& is2);
    void chooseClDevice(uint32_t &maxWorkGroupSize, std::vector<uint32_t>& maxWorkItemSize);
}
