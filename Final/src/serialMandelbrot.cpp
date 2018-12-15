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
static const auto MAX_ITERATIONS = 100;

static const auto ZOOM = 1.0f;
static const auto CENTER = std::make_pair(0, 0);
static const auto ORDER = 3.0f;
static const auto MAX_ORDER = 11.0f;

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
std::vector<uint8_t> fractal(const uint32_t width, const uint32_t height, const float left, const float top, const float xSide, const float ySide, const int maxCount, const float order, const float bailout, std::vector<uint8_t> colors)
{
    auto pixels = std::vector<uint8_t>(width * height * 3, 0);
    // setting up the xscale and yscale 
    const auto xScale = xSide / width;
    const auto yScale = ySide / height;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < height; y++)
    {
        for (auto x = 0; x < width; x++)
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
            while (abs(complexCalculated) < bailout && count < maxCount)
            {
                complexCalculated = pow(complexCalculated, order) + complexConstant;

                // Increment count 
                count = count + 1;
            }

            // To display the created fractal 
            const auto pixelIndex = (y * width + x) * 3;

            if (count == maxCount)
            {
                pixels[pixelIndex] = 0;
                pixels[pixelIndex + 1] = 0;
                pixels[pixelIndex + 2] = 0;
            }
            else
            {
                const auto logZn = static_cast<float>(log(abs(complexCalculated)));
                const auto nu = static_cast<float>(log(logZn / log(bailout)) / log(order));
                const auto adjustedCount = 1.0f - nu;

                const int index1 = (static_cast<int>(floor(adjustedCount)) + count + 1) % (colors.size() / 3);
                const int index2 = (static_cast<int>(floor(adjustedCount)) + count) % (colors.size() / 3);

                //printf("%i %i mod %i -> %i %i\n", (saveState.count + 1), saveState.count, numColors, index1, index2);

                const auto colorIndex1 = 3 * index1;
                const auto colorIndex2 = 3 * index2;

                // Get all the colors based off of what was given to us, and the one plus of what was given to us.
                const auto red1 = colors[colorIndex1];
                const auto green1 = colors[colorIndex1 + 1];
                const auto blue1 = colors[colorIndex1 + 2];

                const auto red2 = colors[colorIndex2];
                const auto green2 = colors[colorIndex2 + 1];
                const auto blue2 = colors[colorIndex2 + 2];

                // Find the weightings for the smoothing.
                const auto weight1 = adjustedCount;
                const auto weight2 = 1.0f - weight1;

                // Smooth each of the lanes.
                const auto red = static_cast<unsigned char>(red1 * weight1 + red2 * weight2);
                const auto green = static_cast<unsigned char>(green1 * weight1 + green2 * weight2);
                const auto blue = static_cast<unsigned char>(blue1 * weight1 + blue2 * weight2);

                pixels[pixelIndex] = red;
                pixels[pixelIndex + 1] = green;
                pixels[pixelIndex + 2] = blue;
            }
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


    SDL_DisplayMode dm;
    if (SDL_GetCurrentDisplayMode(0, &dm) != 0)
    {
        std::cout << "SDL_GetCurrentDisplayMode Error: " << SDL_GetError() << std::endl;
        return 1;
    }
    const auto width = 512; // dm.w;
    const auto height = 512; // dm.h;

    // Create the SDL window that we will use.
    SDL_Window* win = SDL_CreateWindow("Mandelbrot Set", 0, 0, width, height, SDL_WINDOW_SHOWN);
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
        return {};
    }

    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    if (tex == nullptr)
    {
        std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return {};
    }

    // Get the Screen Ratio
    const auto screenRatio = static_cast<float>(width) / height;

    // Center it and set the zoom level.
    // I don't know why this number is needed, but it centers it.
    const auto left = (-DEFAULT_FIT + 0.28125f) / ZOOM - CENTER.first;
    const auto top = -DEFAULT_FIT / 2 / ZOOM + CENTER.second;
    const auto xSide = 2.5f * screenRatio / ZOOM;
    const auto ySide = 2.5f / ZOOM;


    const std::vector<uint8_t> colors = { 
        255, 0, 0, // Red
        0, 255, 0, // Green
        0, 0, 255  // Blue
    };

    const auto startTime = std::chrono::system_clock::now();

    auto initialState = generateZeroState(left, top, xSide, ySide);
    for (auto order = 1.0f; order < MAX_ORDER; order += 0.1f)
    {

        const auto bailout = std::pow(std::pow(FLT_MAX, 1.0f / (order + 2.0f)), 1.0f / 2.0f);

        auto pixelColor = fractal(width, height, left, top, xSide, ySide, MAX_ITERATIONS, order, bailout, colors);

        void* pixels = nullptr;
        auto pitch = 0;
        if (SDL_LockTexture(tex, nullptr, &pixels, &pitch) != 0)
        {
            SDL_DestroyTexture(tex);
            SDL_DestroyRenderer(ren);
            SDL_DestroyWindow(win);
            std::cout << "SDL_LockTexture Error: " << SDL_GetError() << std::endl;
            SDL_Quit();
            return {};
        }

        // Push the pixels to the texture.
        memcpy(pixels, pixelColor.data(), pitch * height);

        // Unlock the texture so that it updates.
        SDL_UnlockTexture(tex);

        //First clear the renderer
        SDL_RenderClear(ren);
        //Draw the texture
        SDL_RenderCopy(ren, tex, nullptr, nullptr);
        //Update the screen
        SDL_RenderPresent(ren);
    }

    const auto endTime = std::chrono::system_clock::now();

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    std::chrono::duration<double> diff = endTime - startTime;

    std::cout << "Run Time Mandelbrot for " << std::to_string(MAX_ITERATIONS) << " = " << diff.count() << " seconds" << std::endl;

    return 0;
}
