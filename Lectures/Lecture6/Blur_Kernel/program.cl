__constant float gaussBlurFilter[25] = {
1.0f/273.0f, 4.0f/273.0f, 7.0f/273.0f, 4.0f/273.0f, 1.0f/273.0f,
4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f, 4.0f/273.0f,
7.0f/273.0f, 26.0f/273.0f, 41.0f/273.0f, 26.0f/273.0f, 7.0f/273.0f,
4.0f/273.0f, 16.0f/273.0f, 26.0f/273.0f, 16.0f/273.0f, 4.0f/273.0f,
1.0f/273.0f, 4.0f/273.0f, 7.0f/273.0f, 4.0f/273.0f, 1.0f/273.0f
};

__constant int filterWidth = 5;

constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_LINEAR;

__kernel void img_conv_filter(
__read_only image2d_t inputImg, 
__write_only image2d_t outputImg, 
int cols, 
int rows
)
{
    // use global IDs for output coords
    int x = get_global_id(0); // columns
    int y = get_global_id(1); // rows
    int halfWidth = (int)(filterWidth/2); // auto-round nearest int ???
    float4 sum = (float4)(0);
    int filtIdx = 0; // filter kernel passed in as linearized buffer array
    int2 coords;
    for(int i = -halfWidth; i <= halfWidth; i++) // iterate filter rows
    {
        coords.y = y + i;
        for(int j = -halfWidth; j <= halfWidth; j++) // iterate filter cols
        {
            coords.x = x + j;
            //float4 pixel = convert_float4(read_imageui(inputImg, sampler, coords)); // operate element-wise on all 3 color components (r,g,b)
            float4 pixel = read_imagef(inputImg, sampler, coords); // operate element-wise on all 3 color components (r,g,b)
            filtIdx++;
            sum += pixel * (float4)(gaussBlurFilter[filtIdx],gaussBlurFilter[filtIdx],gaussBlurFilter[filtIdx],1.0f); // leave a-channel unchanged
        }
    }
    //write resultant filtered pixel to output image
    coords = (int2)(x,y);
    //write_imageui(outputImg, coords, convert_uint4(sum));
    write_imagef(outputImg, coords, sum);
}
