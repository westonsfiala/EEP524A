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
static auto MAXCOUNT = 0;
// Function to draw mandelbrot set 
std::vector<uint8_t> fractal(const float left, const float top, const float xside, const float yside, const int xMax, const int yMax)
{
    auto pixels = std::vector<uint8_t>(xMax * yMax, 0);

    // setting up the xscale and yscale 
    const auto xscale = xside / xMax;
    const auto yscale = yside / yMax;

    // scanning every point in that rectangular area. 
    // Each point represents a Complex number (x + yi). 
    // Iterate that complex number 
    for (auto y = 0; y < yMax-1; y++) {
        for (auto x = 0; x < xMax-1; x++)
        {
            // c_real 
            const auto cx = x * xscale + left;

            // c_imaginary 
            const auto cy = y * yscale + top;

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
            while ((zx * zx + zy * zy < 4) && (count < MAXCOUNT))
            {
                // Calculate Mandelbrot function 
                // z = z*z + c where z is a complex number 

                // tempx = z_real*_real - z_imaginary*z_imaginary + c_real 
                const auto tempx = zx * zx - zy * zy + cx;

                // 2*z_real*z_imaginary + c_imaginary 
                zy = 2 * zx * zy + cy;

                // Updating z_real = tempx 
                zx = tempx;

                // Increment count 
                count = count + 1;
            }

            // Take the count and turn it into a number between 0-255
            auto adjustedCount = 255 * count / MAXCOUNT;

            // To display the created fractal 
            const auto index = y * xMax + x;
            pixels[index] = adjustedCount;
        }
    }

    return pixels;
}

// Driver code 
int main(int argc, char **argv)
{
    // Initialize the SDL library
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create the SDL window that we will use.
    SDL_Window *win = SDL_CreateWindow("Hello World!", 0, 0, xMax, yMax, SDL_WINDOW_SHOWN);
    if (win == nullptr) {
        std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create the Renderer that will render our image into the window.
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (ren == nullptr) {
        SDL_DestroyWindow(win);
        std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    for (auto i = 1; i < 100; ++i)
    {
        MAXCOUNT = i;
        // setting the left, top, xside and yside 
        // for the screen and image to be displayed 
        const auto left = -1.5f;
        const auto top = -0.5f;
        const auto xside = 1.0f;
        const auto yside = 1.0f;

        // Function calling 
        auto pixelColor = fractal(left, top, xside, yside, xMax, yMax);

        std::vector<uint8_t> pixelMap;

        for (auto pixel : pixelColor)
        {
            // Red
            pixelMap.push_back(pixel);
            // Blue
            pixelMap.push_back(pixel);
            // Green
            pixelMap.push_back(pixel);
        }

        // Create our surface from the 
        SDL_Surface* surf = SDL_CreateRGBSurfaceFrom(static_cast<void*>(pixelMap.data()), xMax, yMax, 24, xMax * 3, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000);

        SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (tex == nullptr) {
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