constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_MIRRORED_REPEAT | CLK_FILTER_LINEAR;

__kernel void img_rotate(
__read_only image2d_t inputImg, 
__write_only image2d_t outputImg, 
int imgWidth, 
int imgHeight, 
float theta
)
{
    // use global IDs for output coords
    int x = get_global_id(0);
    int y = get_global_id(1);
    // compute image center
    float x0 = imgWidth/2.0f;
    float y0 = imgHeight/2.0f;
    // compute WI location relative to image center
    int xprime = x-x0;
    int yprime = y-y0;
    // compute sin and cos
    float sinTheta = sin(theta*M_PI_F/180.f);
    float cosTheta = cos(theta*M_PI_F/180.f);
    // compute input location
    float2 readCoord;
    readCoord.x = xprime*cosTheta - yprime*sinTheta + x0;
    readCoord.y = xprime*sinTheta + yprime*cosTheta + y0;
    // read input image
    float value = read_imagef(inputImg, sampler, readCoord).x; // return only x component of float4 (monochromatic image)
    // write output image
    // write to all R-G-B components, will convert from 32-bit uint to 8-bit uints?
    write_imagef(outputImg, (int2)(x,y), (float4)(value, value, value, 0.f));
}