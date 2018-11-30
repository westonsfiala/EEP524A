// C++ implementation for mandelbrot set fractals 
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

static const auto xMax = 1920;
static const auto yMax = 1080;
static auto MAX_ITERATIONS = 1000;

// Function to draw mandelbrot set 
std::vector<uint8_t> fractal(const float left, const float top, const float xSide, const float ySide, const int xMax, const int yMax, const int maxCount)
{
    auto pixels = std::vector<uint8_t>(xMax * yMax, 0);

    // setting up the xscale and yscale 
    const auto xScale = xSide / xMax;
    const auto yScale = ySide / yMax;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < yMax; y++)
    {
        for (auto x = 0; x < xMax; x++)
        {
            // c_real 
            const auto cx = x * xScale + left;

            // c_imaginary 
            const auto cy = y * yScale + top;

            // z_real 
            float zx = 0;

            // z_imaginary 
            float zy = 0;
            auto count = 0;

            // Calculate whether c(c_real + c_imaginary) belongs 
            // to the Mandelbrot set or not and draw a pixel 
            // at coordinates (x, y) accordingly 
            // If you reach the Maximum number of iterations 
            // and If the distance from the origin is 
            // greater than 2 exit the loop 
            while ((zx * zx + zy * zy < (1 << 16)) && (count < maxCount))
            {
                // Calculate Mandelbrot function 
                // z = z*z + c where z is a complex number 

                // tempX = z_real*_real - z_imaginary*z_imaginary + c_real 
                const auto tempX = zx * zx - zy * zy + cx;

                // 2*z_real*z_imaginary + c_imaginary 
                zy = 2 * zx * zy + cy;

                // Updating z_real = tempX 
                zx = tempX;

                // Increment count 
                count = count + 1;
            }

            auto adjustedCount = static_cast<float>(count);
            // Used to avoid floating point issues with points inside the set.
            if (count < maxCount)
            {
                // sqrt of inner term removed using log simplification rules.
                const auto logZn = static_cast<float>(log(zx * zx + zy * zy) / 2);
                const auto nu = static_cast<float>(log(logZn / log(2)) / log(2));
                // Rearranging the potential function.
                // Dividing log_zn by log(2) instead of log(N = 1<<8)
                // because we want the entire palette to range from the
                // center to radius 2, NOT our bailout radius.
                adjustedCount = count + 1 - nu;
            }

            // To display the created fractal 
            const auto index = y * xMax + x;

            pixels[index] = static_cast<uint8_t>(255 * adjustedCount / maxCount);
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
    SDL_Window* win = SDL_CreateWindow("Hello World!", 0, 0, xMax, yMax, SDL_WINDOW_SHOWN);
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

    for (auto i = 5; i < MAX_ITERATIONS; ++i)
    {
        // setting the left, top, xside and yside 
        // for the screen and image to be displayed 
        const auto left = -2.5f;
        const auto top = -1.25f;
        const auto xside = 2.5f * static_cast<float>(xMax) / yMax;
        const auto yside = 2.5f;

        // Function calling 
        auto pixelColor = fractal(left, top, xside, yside, xMax, yMax, i);

        auto pixelMap = std::vector<uint8_t>();
        for (auto pixel : pixelColor)
        {
            pixelMap.push_back(pixel);
            pixelMap.push_back(pixel);
            pixelMap.push_back(pixel);
            pixelMap.push_back(255);
        }

        // Create our surface from the 
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixelMap.data()), xMax, yMax, 32, xMax * 4, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

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

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();

    //stbi_write_png("C:/work/GitHub/EEP524A/Final/output/fractal.png", xMax, yMax, 3, pixelMap.data(), xMax * 3);

    return 0;
}
