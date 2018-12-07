#include <cstdint>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#include "stb_image_write.h"

#include <vector>
#include <ostream>
#include <iostream>
#include <SDL.h>
#include <chrono>
#include <complex>
#include <algorithm>

static const auto MAX_X = 1920;
static const auto MAX_Y = 1080;
static const auto MAX_ITERATIONS = 1000;

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0, 0);
static const auto ORDER = 3.0f;

static const float DEFAULT_FIT = 2.5f;
static const auto ESCAPE_NUM = static_cast<float>(std::min<uint32_t>(UINT32_MAX, 1 << 2 * static_cast<int>(ORDER)));

struct MandelbrotSaveState
{
    std::complex<float> complex;
    std::complex<float> constantComplex;
    uint32_t count;
    float adjustedCount;
};

std::vector<MandelbrotSaveState> generateZeroState(const float left, const float top, const float xSide, const float ySide)
{
    std::vector<MandelbrotSaveState> saveStates;

    // setting up the xscale and yscale 
    const auto xScale = xSide / MAX_X;
    const auto yScale = ySide / MAX_Y;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < MAX_Y; y++)
    {
        for (auto x = 0; x < MAX_X; x++)
        {
            MandelbrotSaveState saveState;
            saveState.constantComplex = std::complex<float>(x * xScale + left, y * yScale + top);
            saveState.complex = std::complex<float>(0, 0);
            saveState.count = 0;
            saveState.adjustedCount = 0;

            saveStates.push_back(saveState);
        }
    }

    return saveStates;
}

// Calculate whether c(c_real + c_imaginary) belongs 
// to the Mandelbrot set or not and draw a pixel 
// at coordinates (x, y) accordingly 
// If you reach the Maximum number of iterations 
// and If the distance from the origin is 
// greater than 2 exit the loop 
void runFractal(MandelbrotSaveState& saveState, const uint32_t& maxCount)
{
    while (abs(saveState.complex) < ESCAPE_NUM && saveState.count < maxCount)
    {
        // Perform the next calculation.
        saveState.complex = pow(saveState.complex, ORDER) + saveState.constantComplex;

        // Increment count 
        saveState.count++;
    }

    saveState.adjustedCount = static_cast<float>(saveState.count);

    // Used to avoid floating point issues with points inside the set.
    if (saveState.count < maxCount)
    {
        const auto logZn = static_cast<float>(log(abs(saveState.complex)));
        const auto nu = static_cast<float>(log(logZn / log(ESCAPE_NUM)) / log(ORDER));
        // Rearranging the potential function.
        // Dividing log_zn by log(2) instead of log(N = 1<<8)
        // because we want the entire palette to range from the
        // center to radius 2, NOT our bailout radius.
        saveState.adjustedCount = saveState.count + 1 - nu;
    }
}

// Function to draw mandelbrot set 
std::vector<uint8_t> fractalSaveState(std::vector<MandelbrotSaveState>& lastStates, const int& maxCount)
{
    auto pixels = std::vector<uint8_t>(lastStates.size(), 0);

    auto index = 0;

    for (auto& saveState : lastStates)
    {
        // Run the fractal calculation.
        runFractal(saveState, maxCount);

        pixels[index] = 255 - static_cast<uint8_t>(255 * saveState.adjustedCount / maxCount);
        ++index;
    }

    return pixels;
}

// Function to draw mandelbrot set 
std::vector<uint8_t> fractal(const float left, const float top, const float xSide, const float ySide, const int maxCount)
{
    auto pixels = std::vector<uint8_t>(MAX_X * MAX_Y, 0);
    // setting up the xscale and yscale 
    const auto xScale = xSide / MAX_X;
    const auto yScale = ySide / MAX_Y;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < MAX_Y; y++)
    {
        for (auto x = 0; x < MAX_X; x++)
        {
            // The constant that is added each time.
            auto complexConstant = std::complex<float>(x * xScale + left, y * yScale + top);

            // The number that will be iterated over.
            auto complexCalculated = std::complex<float>(0, 0);

            auto count = 0;

            // Calculate whether c(c_real + c_imaginary) belongs 
            // to the Mandelbrot set or not and draw a pixel 
            // at coordinates (x, y) accordingly 
            // If you reach the Maximum number of iterations 
            // and If the distance from the origin is 
            // greater than 2 exit the loop 
            while (abs(complexCalculated) < ESCAPE_NUM && count < maxCount)
            {
                complexCalculated = pow(complexCalculated, ORDER) + complexConstant;

                // Increment count 
                count = count + 1;
            }

            auto adjustedCount = static_cast<float>(count);
            // Used to avoid floating point issues with points inside the set.
            if (count < maxCount)
            {
                // sqrt of inner term removed using log simplification rules.
                const auto logZn = static_cast<float>(log(abs(pow(complexCalculated.real(), 2) + pow(complexCalculated.imag(), 2))) / 2);
                const auto nu = static_cast<float>(log(logZn / log(ORDER)) / log(ORDER));
                // Rearranging the potential function.
                // Dividing log_zn by log(2) instead of log(N = 1<<8)
                // because we want the entire palette to range from the
                // center to radius 2, NOT our bailout radius.
                adjustedCount = count + 1 - nu;
            }

            // To display the created fractal 
            const auto index = y * MAX_X + x;

            pixels[index] = 255 - static_cast<uint8_t>(255 * adjustedCount / maxCount);
        }
    }

    return pixels;
}

// Driver code 
int main(int argc, char** argv)
{
    // Initialize the SDL library
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create the SDL window that we will use.
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Set", 0, 0, MAX_X, MAX_Y, SDL_WINDOW_SHOWN);
    if (win == nullptr)
    {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create the Renderer that will render our image into the window.
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr)
    {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Get the Screen Ratio
    const auto screenRatio = static_cast<float>(MAX_X) / MAX_Y;

    // Center it and set the zoom level.
    // I don't know why this number is needed, but it centers it.
    const auto left = (-DEFAULT_FIT + 0.28125f) / ZOOM - CENTER.first;
    const auto top = -DEFAULT_FIT / 2 / ZOOM + CENTER.second;
    const auto xSide = 2.5f * screenRatio / ZOOM;
    const auto ySide = 2.5f / ZOOM;

    const auto startTime = std::chrono::system_clock::now();

    auto initialState = generateZeroState(left, top, xSide, ySide);
    for (auto i = 5; i < MAX_ITERATIONS; ++i)
    {
        // Function calling 
        auto pixelColor = fractalSaveState(initialState, i);

        auto pixelMap = std::vector<uint8_t>();
        for (auto pixel : pixelColor)
        {
            pixelMap.push_back(pixel);
            pixelMap.push_back(pixel);
            pixelMap.push_back(pixel);
            pixelMap.push_back(255);
        }

        // Create our surface from the 
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixelMap.data()), MAX_X, MAX_Y, 32, MAX_X * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (tex == nullptr)
        {
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            std::cout << "SDL_CreateTextureFromSurface Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return 1;
        }

        //First clear the renderer
        SDL_RenderClear(ren);
        //Draw the texture
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        //Update the screen
        SDL_RenderPresent(ren);
        // Get rid of the texture
        SDL_DestroyTexture(tex);
    }

    const auto endTime = std::chrono::system_clock::now();

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    std::chrono::duration<double> diff = endTime - startTime;

    std::cout << "Run Time Mandelbrot for " << std::to_string(MAX_ITERATIONS) << " = " << diff.count() << " seconds" << std::endl;

    return 0;
}
